# SPDX-License-Identifier: GPL-3.0-or-later
"""Headless computer-only game: python -m alpha_smac --headless N.

Starts a new game (or loads one), hands the player's faction to the
computer player (asmac_set_human(player, 0)) so every End Turn plays one
whole round of the engine's AI for all factions, plays N turns and prints a
summary. Nothing here decides anything: the moves are the engine's
(Thinker AI port).
"""
import time

from .game import Game, Status


def summary_lines(g):
    out = []
    for f in g.factions(alive_only=False):
        out.append(f'FACTION {f.id} {f.noun or "-"} alive={int(f.alive)} human={int(f.human)} bases={f.bases} units={f.units} '
                   f'techs={f.techs_known} credits={f.credits}')
    return out


def play_headless(g, turns, emit=print, every=10, keep_player=False):
    """Play `turns` End Turns on an already started Game. Returns a dict."""
    if not keep_player and g.player > 0:
        g.set_human(g.player, False)
    g.messages()
    start = time.time()
    played = 0
    msg_count = 0
    status = Status.OK
    for i in range(turns):
        r = g.end_turn()
        status = r.status
        msg_count += len(r.messages)
        played += 1
        if every and (i + 1) % every == 0 or i == 0:
            alive = [f for f in g.factions()]
            emit(f'TURN {g.turn} year={g.year} factions={len(alive)} bases={g.base_count} units={g.vehicle_count} '
                 f'messages={len(r.messages)}')
        if r.status == Status.GAME_OVER or g.game_over:
            emit(f'GAME OVER on turn {g.turn}')
            break
        if r.status != Status.OK:
            emit(f'END TURN failed: {r.status}')
            break
    elapsed = time.time() - start
    for line in summary_lines(g):
        emit(line)
    digest = g.digest()
    emit(f'FINAL turns={played} turn={g.turn} year={g.year} bases={g.base_count} units={g.vehicle_count} '
         f'messages={msg_count} digest={digest:08x} seconds={elapsed:.1f}')
    return dict(turns=played, turn=g.turn, year=g.year, bases=g.base_count, units=g.vehicle_count, digest=digest,
                status=status, messages=msg_count, game_over=g.game_over)
