#!/usr/bin/env python
import dbus

def get_current_track():
    try:
        # Connect to the session bus
        session_bus = dbus.SessionBus()

        # Get the Spotify player object
        spotify = session_bus.get_object("org.mpris.MediaPlayer2.spotify", "/org/mpris/MediaPlayer2")
        
        # Get the properties of the Spotify player
        properties_interface = dbus.Interface(spotify, "org.freedesktop.DBus.Properties")
        metadata = properties_interface.Get("org.mpris.MediaPlayer2.Player", "Metadata")
        
        # Check if the track is playing
        playback_status = properties_interface.Get("org.mpris.MediaPlayer2.Player", "PlaybackStatus")
        
        if playback_status == "Playing":
            track_name = metadata.get("xesam:title", "Unknown Title")
            artist_name = ', '.join(metadata.get("xesam:artist", ["Unknown Artist"]))
            return f'{track_name} - {artist_name}'
            #return f'\033[1m{track_name}\033[22m- {artist_name}'
        else:
            return ''

    except dbus.DBusException as e:
        return f'Error: {str(e)}'
    except Exception as e:
        return f'An unexpected error occurred: {str(e)}'

if __name__ == "__main__":
    print(get_current_track(),end="")
