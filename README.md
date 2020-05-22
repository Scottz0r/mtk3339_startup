# MTK3339 Startup

This is a demo for starting an configuring an [Adafruit Ultimate GPS Breakout](https://www.adafruit.com/product/746). This module is built around a MTK3339 GPS chip.

The issue with many Arduino examples based around the MTK3339 is that the setup and configuration of the GPS module is not very stable in the examples. Many examples starts throwing configuration messages immediately in the `startup()` method; yet, the GPS may not be in a ready state at that point.

The GPS chip sends messages to indicate when the module has booted and is ready to receive messages. The module also sends ACK messages to acknowledge configuration messages. This project shows how to wait for the messages to ensure configuration changes have been made successfully.

The main disadvantage of this method is any GPS fix will be lost on reset. This sketch will power off the GPS for a few milliseconds to force a restart of the GPS module.

This project is for demonstration purposes only, and should be used as a guideline of implementing one's own MTK3339 startup and configuration routine.

## Wiring

The wiring is the same as demoed in [Adafruit's Breakout Arduino Wiring Guide](https://learn.adafruit.com/adafruit-ultimate-gps/arduino-wiring) with the addition of the enable pin. In the `mtk3339_startup.ino` file, the EN pin is wired to digital pin 7 (label 7 on Uno and Mega). This pin is used to fully control when the GPS turns on, giving greatly control of the steps needed to successfully and reliably configure the GPS module.

## Output

Below is sample output after the board is power on or reset (via reset button or Serial Monitor).

```text
Turning off GPS_ENABLE.
Configuring MTK3339...
Turning on GPS_ENABLE.
Waiting for startup messages...
Found other message: "".
Found other message: "$PGACK,105*46".
Found startup MTKGPS text message.
Found startup message 001.
Got MTK3339 startup messages.
Sending 314 (message type config)...
Found when waiting for ACK: $PMTK010,002*2D
Found 314 successful ACK.
Sending 220 (message speed config)...
Found 220 successful ACK.
Setup complete.
```

Afterwards, the GPGGA message will print to the Serial Monitor at 5 second intervals.
