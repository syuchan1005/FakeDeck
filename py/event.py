#!/usr/bin/env python3

import threading

from StreamDeck.DeviceManager import DeviceManager


def key_change_callback(deck, key, state):
    print("Deck {} Key {} = {}".format(deck.id(), key, "down" if state else "up"), flush=True)

def dial_change_callback(deck, dial, event, value):
    print("Deck {} Dial {} {} = {}".format(deck.id(), dial, event, value), flush=True)

def touchscreen_event_callback(deck, evt_type, value):
    print("Deck {} Touchscreen {} = {}".format(deck.id(), evt_type, value), flush=True)

if __name__ == "__main__":
    streamdecks = DeviceManager().enumerate()

    print("Found {} Stream Deck(s).\n".format(len(streamdecks)))

    for index, deck in enumerate(streamdecks):
        deck.open()

        print("Opened '{}' device (serial number: '{}', fw: '{}')".format(
            deck.deck_type(), deck.get_serial_number(), deck.get_firmware_version()
        ))

        # Register callback function for when a key state changes.
        deck.set_key_callback(key_change_callback)
        deck.set_dial_callback(dial_change_callback)
        deck.set_touchscreen_callback(touchscreen_event_callback)

        # Wait until all application threads have terminated (for this example,
        # this is when all deck handles are closed).
        for t in threading.enumerate():
            try:
                t.join()
            except RuntimeError:
                pass