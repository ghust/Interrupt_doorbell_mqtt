# The Wemos Smart doorbell



I started this project because I wanted a notification when someone rang the bell. Nest is expensive!
As we use Home Assistant as our Automation hub and integrate almost everything using MQTT, the logic became pretty simple: we need to make something that sends an MQTT message when a button is pressed. Turns out that's pretty simple.

# Our setup

  - Our doorbell uses 8V AC. This makes it hard to integrate in common setups (5V DC, 12V DC, 220AC are "easier"). The button breaks the circuit between the 8V AC transformer and the chime. Pretty basic stuff.


# What you need:
- A Wemos D1 Mini + Relay shield.
- Soldering Iron
- Some wire
- An USB cable


# Wiring

- Attach the relay shield on the Wemos D1 mini.
Line up the pins, connect them using the pins that were included, give it a little touch of solder.
- On the relay shield, attach one of the chime wires to the center one (comomon), the other to NC pin.
- Attach one of the doorbell wires to ground (G) on the Wemos D1 and the other on the D3 pin.

# Logic

The programming itself is going to be relatively straightforward. 
We're going to listen for *interrupts* on the D3 pin. When that happens, we're going to send a signal to the relay to close, producing the music.
When the button is released, we're going to open the relay.
We also have a throttling mechanism, in order not to get flooded by messages.

The only things you need to do is create the arduino_secrets.h file (example is added) and configure your Wi-Fi and MQTT broker info.

