from threading import Event
from time import sleep
from .alex_control_serial import startSerial, readSerial, writeSerial, closeSerial
from .alex_control_serialize import serialize, deserialize
from .alex_control_constants import (
    TCommandType, TPacketType, TResponseType, TPacket,
    PAYLOAD_PARAMS_COUNT, PAYLOAD_PACKET_SIZE,
    TComms, TResultType, COMMS_MAGIC_NUMBER, COMMS_PACKET_SIZE
)

_EXIT_EVENT = Event()

def receivePacket(exitFlag: Event = _EXIT_EVENT):
    target_packet_size = COMMS_PACKET_SIZE
    buffer = bytearray(target_packet_size)
    buffer_size = 0
    while not exitFlag.is_set():
        res_size, res = readSerial(target_packet_size - buffer_size)
        if res_size != 0:
            buffer[buffer_size:buffer_size + res_size] = res
            buffer_size += res_size
        if buffer_size == target_packet_size:
            res_status, payload = deserialize(buffer)
            if res_status == TResultType.PACKET_OK:
                return TPacket.from_buffer(payload)
            else:
                handleError(res_status)
                return None
    return None

def sendPacket(packetType: TPacketType, commandType: TCommandType, params: list):
    packet_to_send = TPacket()
    packet_to_send.packetType = int(packetType.value) if isinstance(packetType, TPacketType) else int(packetType)
    packet_to_send.command = int(commandType.value) if isinstance(commandType, TCommandType) else int(commandType)
    if params != []:
        packet_to_send.params[0:PAYLOAD_PARAMS_COUNT] = [int(x) for x in params]
    to_comms = serialize(packet_to_send)
    writeSerial(to_comms)

def handleError(res_status: TResultType):
    if res_status == TResultType.PACKET_BAD:
        print("ERROR: Received Bad Packet from Arduino")
    elif res_status == TResultType.PACKET_CHECKSUM_BAD:
        print("ERROR: Received Bad Checksum from Arduino")
    else:
        print("ERROR: Unknown Error in Processing Packet")

def printPacket(packet: TPacket):
    print(f"Packet Type: {packet.packetType}")
    print(f"Command: {packet.command}")
    print(f"Data: {packet.data}")
    params = [x for x in packet.params]
    print(f"Params: {params}")

def parseParams(p, num_p, inputMessage):
    if num_p == 0:
        return [0] * PAYLOAD_PARAMS_COUNT
    elif len(p) >= num_p:
        return p[:num_p] + [0] * (PAYLOAD_PARAMS_COUNT - num_p)
    elif len(p) < num_p and inputMessage is not None:
        params_str = input(inputMessage)
        split_input = params_str.split(" ")
        return parseParams(split_input, num_p, None)
    else:
        return None

def parseUserInput(input_str: str, exitFlag: Event = _EXIT_EVENT):
    split_input = [x for x in input_str.strip().split(" ") if x != ""]
    if len(split_input) < 1:
        return print(f"{input_str} is not a valid command")
    command = split_input[0]
    packetType = TPacketType.PACKET_TYPE_COMMAND

    if command == "f":
        commandType = TCommandType.COMMAND_FORWARD
        params = parseParams(split_input[1:], 2, "Enter distance in cm and power in %:\n")
        return (packetType, commandType, params) if params else print("Invalid Parameters")
    elif command == "b":
        commandType = TCommandType.COMMAND_REVERSE
        params = parseParams(split_input[1:], 2, "Enter distance in cm and power in %:\n")
        return (packetType, commandType, params) if params else print("Invalid Parameters")
    elif command == "l":
        commandType = TCommandType.COMMAND_TURN_LEFT
        params = parseParams(split_input[1:], 2, "Enter degrees to turn left and power in %:\n")
        return (packetType, commandType, params) if params else print("Invalid Parameters")
    elif command == "r":
        commandType = TCommandType.COMMAND_TURN_RIGHT
        params = parseParams(split_input[1:], 2, "Enter degrees to turn right and power in %:\n")
        return (packetType, commandType, params) if params else print("Invalid Parameters")
    elif command == "s":
        commandType = TCommandType.COMMAND_STOP
        params = parseParams(split_input[1:], 0, None)
        return (packetType, commandType, params)
    elif command == "c":
        commandType = TCommandType.COMMAND_CLEAR_STATS
        params = parseParams(split_input[1:], 0, None)
        return (packetType, commandType, params)
    elif command == "g":
        commandType = TCommandType.COMMAND_GET_STATS
        params = parseParams(split_input[1:], 0, None)
        return (packetType, commandType, params)
    elif command == "o":
        commandType = TCommandType.COMMAND_OPEN
        params = parseParams(split_input[1:], 0, None)
        return (packetType, commandType, params)
    elif command == "p":
        commandType = TCommandType.COMMAND_CLOSE
        params = parseParams(split_input[1:], 0, None)
        return (packetType, commandType, params)
    elif command == "q":
        print("Exiting! Setting Exit Flag...")
        print("\n==============CLEANING UP==============")
        exitFlag.set()
        return None
    else:
        return print(f"{command} is not a valid command")

def waitForHelloRoutine():
    sendPacket(TPacketType.PACKET_TYPE_HELLO, TCommandType.COMMAND_STOP, [0] * PAYLOAD_PARAMS_COUNT)
    packet = receivePacket()
    if packet:
        packetType = packet.packetType
        res_type = packet.command
        if (
            packetType == TPacketType.PACKET_TYPE_RESPONSE.value and
            res_type == TResponseType.RESP_OK.value
        ):
            print("Received Hello Response")
            return
    raise Exception(f"Failed to receive proper response from Arduino: Packet Type Mismatch {packetType}")

def handleStatus(packet: TPacket):
    print("\n ------- ALEX STATUS REPORT ------- \n")
    print(f"Left Forward Ticks:\t\t{packet.params[0]}")
    print(f"Right Forward Ticks:\t\t{packet.params[1]}")
    print(f"Left Reverse Ticks:\t\t{packet.params[2]}")
    print(f"Right Reverse Ticks:\t\t{packet.params[3]}")
    print(f"Left Forward Ticks Turns:\t{packet.params[4]}")
    print(f"Right Forward Ticks Turns:\t{packet.params[5]}")
    print(f"Left Reverse Ticks Turns:\t{packet.params[6]}")
    print(f"Right Reverse Ticks Turns:\t{packet.params[7]}")
    print(f"Forward Distance:\t\t{packet.params[8]}")
    print(f"Reverse Distance:\t\t{packet.params[9]}")
    print("\n---------------------------------------\n")

def handlePacket(packet: TPacket):
    if packet.packetType == TPacketType.PACKET_TYPE_COMMAND.value:
        pass
    elif packet.packetType == TPacketType.PACKET_TYPE_RESPONSE.value:
        if packet.command == TResponseType.RESP_OK.value:
            print("Command OK")
        elif packet.command == TResponseType.RESP_STATUS.value:
            handleStatus(packet)
        else:
            print("Arduino is confused")
    elif packet.packetType == TPacketType.PACKET_TYPE_ERROR.value:
        if packet.command == TResponseType.RESP_BAD_PACKET.value:
            print("Arduino received bad magic number")
        elif packet.command == TResponseType.RESP_BAD_CHECKSUM.value:
            print("Arduino received bad checksum")
        elif packet.command == TResponseType.RESP_BAD_COMMAND.value:
            print("Arduino received bad command")
        elif packet.command == TResponseType.RESP_BAD_RESPONSE.value:
            print("Arduino received unexpected response")
        else:
            print("Arduino reports a weird error")
    elif packet.packetType == TPacketType.PACKET_TYPE_MESSAGE.value:
        print(f"Message from Alex: {packet.data.decode(errors='ignore')}")
