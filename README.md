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
The application is available [here](https://github.com/raszymura/BLE_test_tool_py/blob/main/README.md)