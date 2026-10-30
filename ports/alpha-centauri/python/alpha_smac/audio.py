# SPDX-License-Identifier: GPL-3.0-or-later
"""Sound effects and music through pygame.mixer, from the user's own files.

The catalog is the native one (audio_events.h, AUDIO.md): event names, the
default fx file of each event and the owner's alpha-audio.cfg mapping and
volumes, the faction music sets (music/<set>.wav, music/<set>-N.wav) and
the menu loop "opening menu.wav". Effects are looked up as fx/<name>, then
<name>, in the data folder (any case). Anything missing stays silent; if
no audio device opens the whole object is silent.
"""
import ctypes as C
from pathlib import Path

from . import _lib as L
from .view import find_file


def lib():
    return L.load()


class Audio:
    def __init__(self, data_dir, enabled=True, pygame_mod=None):
        self.data_dir = Path(data_dir) if data_dir else None
        self.ok = False
        self.error = ''
        self.played = []          # (event, file) of every effect started, for tests and the log
        self.music_track = None
        self.music_on = True
        self.sfx_on = True
        self._cache = {}
        self._playlist = []
        self._cursor = 0
        self._menu = False
        cfg_path = find_file(self.data_dir, 'alpha-audio.cfg') if self.data_dir else None
        self.cfg = Path(cfg_path).read_bytes() if cfg_path else b''
        levels = (C.c_int * 7)()
        n = lib().asmac_audio_levels(self.cfg, len(self.cfg), levels, 7)
        if n == 0 and self.cfg:  # a bad settings file: the catalog defaults
            self.cfg = b''
            n = lib().asmac_audio_levels(b'', 0, levels, 7)
        self.master, self.sfx_level, self.voice_level, self.music_level = (levels[0], levels[1], levels[2], levels[3])
        self.sfx_on, self.music_on = bool(levels[4]), bool(levels[6])
        self.events = {}
        buf = C.create_string_buffer(80)
        for i in range(lib().asmac_audio_event_count()):
            key = lib().asmac_audio_event_key(i).decode()
            r = lib().asmac_audio_event_file(self.cfg, len(self.cfg), i, buf, 80)
            self.events[key] = buf.value.decode('cp1252') if r == 1 else None
        if not enabled or not self.data_dir:
            self.error = 'audio off'
            return
        try:
            import pygame
            self.pg = pygame_mod or pygame
            if not self.pg.mixer.get_init():
                self.pg.mixer.init(frequency=22050, size=-16, channels=1, buffer=1024)
            self.ok = True
        except Exception as e:  # noqa: BLE001 - no device: silent
            self.error = f'no audio device: {e}'

    # ---- files ----
    def _find(self, rel):
        return find_file(self.data_dir, rel) if self.data_dir else None

    def effect_path(self, event):
        name = self.events.get(event)
        if not name:
            return None
        return self._find('fx/' + name) or self._find(name)

    def available(self):
        """Events that have a file in this install."""
        return {e: p for e in self.events if (p := self.effect_path(e))}

    # ---- playback ----
    def event(self, event):
        """Play the effect of a native AudioEvent name (unit_move, menu_open, ...)."""
        if not self.ok or not self.sfx_on:
            return False
        path = self.effect_path(event)
        if not path:
            return False
        snd = self._cache.get(path)
        if snd is None:
            try:
                snd = self.pg.mixer.Sound(path)
            except Exception:  # noqa: BLE001
                self._cache[path] = False
                return False
            snd.set_volume(self.master * self.sfx_level / 10000.0)
            self._cache[path] = snd
        if snd is False:
            return False
        snd.play()
        self.played.append((event, Path(path).name))
        return True

    def menu_music(self, on=True):
        """The title screen's loop (original_frontend.cpp: "opening menu.wav")."""
        if not self.ok:
            return False
        if not on:
            if self._menu:
                self.pg.mixer.music.stop()
            self._menu = False
            return True
        path = self._find('opening menu.wav')
        if not path or not self.music_on:
            return False
        try:
            self.pg.mixer.music.load(path)
            self.pg.mixer.music.set_volume(self.master * self.music_level / 10000.0)
            self.pg.mixer.music.play(-1)
        except Exception:  # noqa: BLE001
            return False
        self._menu = True
        self.music_track = Path(path).name
        return True

    def game_music(self, faction_stem):
        """The faction's score set, else the default set, else any prepared set
        (MusicPlaylist::build)."""
        self.menu_music(False)
        self._playlist = []
        first = lib().asmac_faction_music_set(faction_stem.encode()).decode().split('.')[0]
        sets = [first]
        sets += ['aset1'] + [lib().asmac_music_set(i).decode() for i in range(lib().asmac_music_set_count())]
        for s in sets:
            for n in range(10):
                rel = f'music/{s}.wav' if n == 0 else f'music/{s}-{n}.wav'
                p = self._find(rel)
                if p and p not in self._playlist:
                    self._playlist.append(p)
            if self._playlist:
                break
        self._cursor = 0
        self.tick()
        return len(self._playlist)

    def tick(self):
        """Start the next playlist track when the previous one ended."""
        if not self.ok or not self.music_on or not self._playlist or self._menu:
            return
        if self.pg.mixer.music.get_busy():
            return
        path = self._playlist[self._cursor % len(self._playlist)]
        self._cursor += 1
        try:
            self.pg.mixer.music.load(path)
            self.pg.mixer.music.set_volume(self.master * self.music_level / 10000.0)
            self.pg.mixer.music.play()
            self.music_track = Path(path).name
        except Exception:  # noqa: BLE001
            self._playlist.remove(path)

    def toggle_music(self):
        self.music_on = not self.music_on
        if self.ok and not self.music_on:
            self.pg.mixer.music.stop()
        return self.music_on

    def toggle_sfx(self):
        self.sfx_on = not self.sfx_on
        return self.sfx_on

    def close(self):
        if self.ok:
            try:
                self.pg.mixer.music.stop()
                self.pg.mixer.quit()
            except Exception:  # noqa: BLE001
                pass
        self.ok = False
