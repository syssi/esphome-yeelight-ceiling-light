# Flashing over UART: the things that go wrong

The per-model pinouts in the README tell you which pads to use. This is about
what happens when you have wired them correctly and it still does not work.

Everything here was hit in practice on a `yeelink.light.ceiling10`, using a
CP2102 USB-serial adapter, and cost hours to work out. None of it is exotic; it
is all first-timer territory, which is exactly why it is written down.

---

## The two that will actually stop you

### 1. A common ground is not optional

Tie the adapter's GND to the board's GND. If you power the board from a separate
supply, all three grounds - adapter, supply, board - must be the same node.

Without it you get a symptom that looks like anything but grounding:

- the boot log **arrives perfectly** in your terminal
- `esptool` reports `Serial data stream stopped: Possible serial noise or
  corruption`
- with `--trace`, it receives **a byte-perfect copy of the frame it just sent**

The mechanism: signalling current needs a return path, and with no ground it
returns through the *other* signal wire. Your own transmission appears at your own
receiver. That is why the echo tracks whatever baud rate you pick - it *is* your
signal, not a reply.

How to recognise it in a trace. This is `esptool` sending SYNC and getting its own
bytes back:

```
Write 46 bytes:  c0 00 08 24 00 00000000 07071220 5555...  c0
Received full packet:  0008240000000000 0707122055555555 ...
```

Espressif's serial protocol specification is explicit that the ROM loader **never
echoes the request**, and that a response always begins with direction byte
`0x01`. A packet coming back with `0x00` - the *request* direction - is your own
transmission, not an answer.

### 2. The adapter's 3.3V rail may not start an ESP32

An ESP32 draws a large current spike at power-on. Powering the board from the
adapter's 3.3V pin can dip the USB 5V rail far enough to reset the USB-serial chip
itself. The port then disappears and reappears, and your terminal says something
like `Error reading from serial device` at the exact moment you switch power on -
which is also the moment the boot log you wanted was printed.

Three fixes, cheapest first:

- **a separate 3.3V supply** for the board, adapter's VCC output switched off,
  grounds common. A **partially discharged** 18650 works well: low internal
  resistance, so the inrush is a non-issue. Note **partially** - see the warning
  below.
- **bulk capacitance** across the board's 3.3V and GND, 100-470uF. Adapter
  regulators often have only ~22uF on the output, which is not enough to hold the
  rail through startup.
- **a different USB port** - rear-panel rather than front-panel, or a powered hub.
  Sometimes the extra headroom is all it takes.

> **If you use a lithium cell, measure it first - every time.** The ESP32's
> absolute maximum on `VDD33` is **3.6V**. A **fully charged 18650 sits at 4.2V and
> will destroy the chip**. A **partially discharged cell around 3.4-3.5V is inside
> spec** - but the same cell after a charge is not, so a cell that was safe
> yesterday is not necessarily safe today.
>
> A bare 18650 will also push tens of amps into a short, so keep the leads short,
> insulate the unused end, and connect the negative last.

---

## Smaller traps, roughly in the order you meet them

**Set the adapter to 3.3V logic, not 5V.** Many adapters have a selector for both
VCC and the bus level. At 5V you put 5V into `GPIO3`. Check it before connecting
anything.

**Cross TX and RX.** Adapter TX to board RX, adapter RX to board TX. Worth stating
because the next item makes a half-working setup look plausible.

**Reading the boot log only proves one direction.** The log travels board TX ->
adapter RX. `esptool` needs the opposite direction, which a readable boot log says
nothing about. If the log works but nothing syncs, suspect the direction you have
not exercised.

**A continuity beeper will not sound through a series resistor - and there is
usually one on TX and RX.** These boards commonly put **100R in series on each
UART line**, between the `TX`/`RX` debug pads and the module's `TXD0`/`RXD0` pins.
A beeper trips below ~50R, so it stays silent while the signal passes perfectly.

So "no beep from the pad to the module pin" does **not** mean the trace is broken.
**Measure resistance, not continuity**, and expect roughly 100R rather than a
short. On an ESP32-WROOM module the pins to check against are **34 (`RXD0`)** and
**35 (`TXD0`)** - and a handy trick is to find `TXD0` first using the pad you know
works, since the boot log proves that path, then check the other pad against its
neighbour.

**Close the terminal before running esptool.** Windows gives serial ports
exclusively to one process. PuTTY holding the port produces
`could not open port 'COM3': PermissionError(13)`.

**`GPIO0` is sampled at reset, not continuously.** Shorting it while the chip is
already running does nothing. Ground it, *then* power on. Leaving it grounded for
the whole session is fine and means an unexpected reset re-enters download mode
rather than booting a half-written image. Espressif also require `GPIO2` to be low
or floating for download mode; grounding it too is harmless.

**Open the terminal before powering up.** The bootloader prints once, at reset. If
the chip is already running you will see nothing until it resets.

**Most adapters here cannot reset the chip for you.** `DTR`/`RTS` are not wired to
the board's `EN`/`GPIO0`, so `esptool` cannot toggle download mode itself - power
cycle by hand. Add `--before no-reset --after no-reset` if it complains about the
reset it could not perform.

---

## Reading esptool's errors

The wording is misleading, so it is worth knowing what the code actually checks
(`esptool/loader.py`):

| message | what it means |
| ------- | ------------- |
| `No serial data received.` | nothing came back at all |
| `Serial data stream stopped: Possible serial noise or corruption.` | **one complete frame was read, then nothing more** - so something replied once and then went quiet |

The second is not really about noise. Getting it after a byte-perfect echo means
the "reply" was your own transmission and the chip never actually answered - see
requirement 1 above.

`--trace` is the tool that settles all of this. It prints every byte written and
read, and the direction byte alone distinguishes "the chip answered" from "I am
talking to myself".

## A loopback test, and the trap in it

Shorting the adapter's own TX to its own RX and typing in a terminal proves the
adapter can send and receive: characters echo.

The trap is using it to decide *where* a loop is. Disconnecting only the adapter's
TX also stops the echo, because it removes the driver - whichever end is looping
it. To test the adapter alone, **disconnect both wires from the board**, leave them
plugged into the adapter and physically apart, and type. An echo then can only be
the adapter.

## Expect silence from a healthy ESPHome boot

Configs here set `logger: baud_rate: 0`, which disables the UART logger. A working
boot therefore looks like bootloader chatter and then **nothing**. That is success,
not a hang - confirm it by looking for the device on your network.

The bootloader's own output cannot be disabled, so it is always available for
diagnosis regardless of what the application does.

## Where to write, and how to get back

Read the partition table before writing anything:

```
esptool read-flash 0x8000 0xc00 ptable.bin
```

Application images go to an OTA slot - `0x10000` on a `ceiling10` - **not** to
`0x0`, which is the bootloader. `otadata` decides which slot boots, and stock
survives in the slot the updater did not write, so reverting is usually a matter
of pointing `otadata` back rather than reflashing. See
[flashing-over-the-network.md](flashing-over-the-network.md) for the layout and
the `otadata` mechanics.
