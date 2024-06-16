# BLE Calculator Application with nRF Connect SDK

This project demonstrates the creation of a BLE-enabled calculator application using the Nordic Semiconductor nRF Connect SDK.
The application includes the following functionalities:

- **BLE Advertisement with Manufacturer Specific Data.** The device broadcasts itself with additional data indicating the number of seconds since the platform was reset.

- **Calculator Data Service.** A custom service allowing basic calculator functionalities:
    - Supports floating-point (32-bit) operations with FPU.
    - Supports fixed-point (Q31) operations.
    - Operations include addition, subtraction, multiplication, division, zeroing, and format conversion.
    - Supports both single-argument (using the result from the previous operation) and dual-argument operations.
    - Contains at least two characteristics for binary data exchange:
        - Write characteristic for arguments and operations.
        - Notification characteristic for the calculated result.
    - The calculator engine runs in a dedicated thread. Data read from the characteristics is passed as a task structure between the thread handling notifications and the calculator engine thread.

### Test Tool
A PC application written in Python using the bleak library for BLE communication.
Provides an interactive terminal for performing calculations in both supported numeric modes.
The application is available [here](https://github.com/raszymura/BLE_test_tool_py)

## Definitions
**Generic Attribute Profile (GATT)** defines the necessary sub-procedures for using the ATT layer.\
**Attribute Protocol (ATT)** allows a device to expose certain pieces of data to another device.\
**Advertising:** The process of transmitting advertising packets, either just to broadcast data or to be discovered by another device.\
**Manufacturer Specific Data** (BT_DATA_MANUFACTURER_DATA). This is a popular type that enables companies to define their own custom advertising data.\
**Scanning:** The process of listening for advertising packets.\
**Connection-oriented communication:** When there is a dedicated connection between devices, forming bi-directional communication.\
**Central:** A device role that scans and initiates connections with peripherals. In our case PC.\
**Peripheral:** A device role that advertises and accepts connections from centrals. In our case nRF52833 Devkit.\
**GATT server:** Device that stores data and provides methods for the GATT client to access the data. In our case nRF52833 Devkit.
**GATT client:** Device that accesses the data on the GATT server, through specific GATT operations. In our case PC.\
**Attribute:** A standardized data representation format defined by the ATT protocol.\
**Service discovery:** The process in which a GATT client discovers the services and characteristics in an attribute table hosted by a GATT server.

## Services and Characteristics
**Services** are used to break data up into logical entities, and contain specific chunks of data called characteristics. A service can have one or more characteristics, and each service distinguishes itself from other services by means of a unique numeric ID called a UUID, which can be either 16-bit (for officially adopted BLE Services) or 128-bit (for custom services).\
**Characteristic** encapsulates a single data point. Similarly to Services, each Characteristic distinguishes itself via a pre-defined 16-bit or 128-bit UUID, and you're free to use the standard characteristics defined by the Bluetooth SIG (which ensures interoperability across and BLE-enabled HW/SW) or define your own custom characteristics which only your peripheral and SW understands.
Characteristics are also used to send data back to the BLE peripheral (are also able to write to characteristic).


Service - **Calculator Data Service**, 2 characteristics:
- **Write** arguments and operations
- **Notify** result of the operation (+CCCD)

## Data access
### Client-initiated operations
Client-initiated operations are GATT operations where the client requests data from the GATT server. The client can request to either read or write to an attribute, and in the case of the latter, it can choose whether to receive an acknowledgment from the server.

**Read**

If a client wishes to read a certain value stored in an attribute on a GATT server, the client sends a read request to the server. To which the server responds by returning the attribute value.


**Write**

If the client wishes to write a certain value to an attribute, it sends a write request and provides data that matches the same format of the target attribute. If the server accepts the write operation, it responds with an acknowledgement.


**Write without response**

If this operation is enabled, a client can write data to an attribute without waiting for an acknowledgment from the server. This is a non-acknowledged write operation that can be used when quick data exchange is needed.


### Server-initiated operations
The other category of GATT operations are server-initiated operations, where the server sends information directly to the client, without receiving a request first. In this case, the server can either notify or indicate.


**Notify**

A Notify operation is used by the server to automatically push the value of a certain attribute to the client, without the client asking for it. 
Notifications require no acknowledgment back from the client.


**Indicate**

Similar to the Notify operation, Indicate will also push the attribute value directly to the client. However, in this case, an acknowledgment from the client is required. Because of the acknowledgement requirement, you can only send one Indication per connection interval, meaning Indications are slower than notifications.
