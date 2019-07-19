# Modbus TCP Slave Example Cpp

A basic Modbus TCP Slave example written in CPP using the [CAS Modbus Stack](https://store.chipkin.com/services/stacks/modbus-stack)

## Example output

```txt
FYI: Modbus Stack version: 2.3.11.0
FYI: Modbus stack init, successfuly
FYI: ConnectionMax: 5
FYI: Waiting on TCP connection TCP port=[502]...
FYI: Got a connection from IP address=[127.0.0.1] port=[15025]...
FYI: The number of connections changed to [1]
FYI: RX: 00 00 00 00 00 06 00 03 00 00 00 05
FYI: Get Modbus Value. slaveAddress=[0], function=[3], startingAddress=[0], length=[5], dataSize=[250]
FYI: TX: 00 00 00 00 00 0D 00 03 0A 00 00 00 01 00 02 00 03 00 04
FYI: RX: 00 00 00 00 00 06 00 06 00 00 04 D2
FYI: Set Modbus Value. slaveAddress=[0], function=[6], startingAddress=[0], length=[1], dataSize=[2]
FYI: TX: 00 00 00 00 00 06 00 06 00 00 04 D2
```txt


## Building

1. Copy *CASModbusStack_Win32_Debug.dll*, *CASModbusStack_Win32_Release.dll*, *CASModbusStack_x64_Debug.dll*, and *CASModbusStack_x64_Release.dll* from the [CAS Modbus Stack](https://store.chipkin.com/services/stacks/modbus-stack) project  into the /bin/ folder.
2. Use [Visual Studios 2019](https://visualstudio.microsoft.com/vs/) to build the project. The solution can be found in the */ModbusTCPSlaveExampleCpp/* folder.

Note: The project is automaticly build on every checkin using GitlabCI.
