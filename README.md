# STM32 Laser Controller (UART + DMA)

## Current Status
* **2026-04-02**: 
    - **New Feature**: Added "ON/OFF" command support via Serial to control Laser Enable (PB6).
    - **Stability Fix**: Fixed the system hang issue. Added ORE/FE error clearing in USART1_IRQHandler and increased OLED refresh interval.
    - **Code Cleanup**: Switched all comments to English for better readability.

## Serial Commands
* `ON`: Enable Laser Power.
* `OFF`: Emergency Shutdown.
* `0-4095`: Set DAC Output Voltage.

## Problems Existing
* **PD Inside the LD**: Currently not measuring output current. Need to check the hardware connection.