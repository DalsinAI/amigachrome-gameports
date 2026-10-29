# SPDX-License-Identifier: GPL-3.0-or-later
"""The Python API over the C shim: new game, records, orders, production,
research, save/load round trip, errors."""
import pytest

import alpha_smac as ac
from alpha_smac import Cmd, Item, Status


@pytest.fixture
def game(rules):
    g = ac.new_game(rules, seed=777, faction=1, difficulty=2, map_size=2)
    yield g
    g.close()


def test_new_game_records(game):
    assert game.loaded and game.player == 1 and game.turn == 1 and game.year == 2101
    assert game.digest() == 0x359aba11  # = "A VIEW ... digest=359aba11" of the native play test
    w, h = game.map_size()
    assert (w, h) == (80, 80)
    _, _, tiles = game.tiles()
    assert sum(t.valid for t in tiles) == w * h // 2
    facs = game.factions()
    assert facs[0].id == 1 and facs[0].human and facs[0].noun == 'Noun1'
    own = game.units(own=True)
    assert {u.name for u in own} == {'Patrol', 'Work Crew'}
    b = game.bases(faction=1)[0]
    assert b.name == 'First base' and game.base_at(b.x, b.y) == b.id
    t = game.tile(b.x, b.y)
    assert t.base == b.id and t.items & ac.TileBits.BASE
    msgs = game.messages()
    assert any(m.text == 'Your turn: Noun1, turn 1' for m in msgs)
    assert game.messages() == []


def test_orders_production_research_save_load(game, tmp_path, rules):
    game.messages()
    moved = None
    for unit in game.units(own=True):
        v = unit.id
        for d in range(8):
            ok, x, y = game.step_target(v, d)
            if not ok or game.hostile_at(x, y):
                continue
            r = game.move(v, d)
            after = game.unit(v)
            if r == Status.OK and (after.x, after.y) == (x, y):
                moved = (v, unit.x, unit.y)
                break
            # An accepted order may leave the unit in place with its moves
            # spent (as play_control's wander() allows); a refusal must not.
            assert r != Status.OK or after.moves_left < unit.moves_left or (after.x, after.y) == (unit.x, unit.y)
        if moved:
            break
    assert moved and (game.unit(moved[0]).x, game.unit(moved[0]).y) != moved[1:]
    assert any('moved to' in m.text for m in game.messages())
    base = game.bases(faction=1)[0].id
    panel = game.production_panel(base)
    colony = next(panel.items[i].id for i in range(panel.count) if panel.items[i].kind == Item.COLONY)
    assert game.set_production(base, colony) == Status.OK
    assert game.base(base).current_item == colony
    rp = game.research_panel()
    assert rp.count > 1
    r = game.research(rp.ids[1])
    if r == Status.NEEDS_CONFIRM:
        r = game.research(rp.ids[1], confirm=True)
    assert r == Status.OK and game.research_panel().current == rp.ids[1]
    assert game.run_text('endturn') == Status.OK and game.turn == 2
    path = tmp_path / 'g.SAV'
    game.save(path)
    g2, g3 = ac.load(rules, path), ac.load(rules, path)
    assert (g2.turn, g2.vehicle_count, g2.base_count, g2.credits) == (game.turn, game.vehicle_count, game.base_count, game.credits)
    assert [(u.x, u.y, u.unit_id) for u in g2.units()] == [(u.x, u.y, u.unit_id) for u in game.units()]
    # Two loads of one save stay identical when continued (the native B/C check).
    assert g2.digest() == g3.digest()
    assert g2.end_turn().status == Status.OK and g3.end_turn().status == Status.OK and g2.turn == 3
    assert g2.digest() == g3.digest()
    g2.close()
    g3.close()


def test_commands_and_errors(rules, tmp_path):
    c = ac.parse_command('attack 3 10 12')
    assert c and c.kind == Cmd.ATTACK and (c.a, c.b, c.c) == (3, 10, 12)
    assert ac.parse_command('fly 1') is None
    assert ac.command_name(Cmd.END_TURN) == 'endturn'
    assert ac.status_name(Status.NEEDS_CONFIRM) == 'needs confirmation'
    g = ac.Game()
    assert g.run(Cmd.END_TURN) == Status.NO_GAME
    assert g.load_rules(tmp_path, True) == Status.IO and g.rules_error
    with pytest.raises(FileNotFoundError):
        ac.find_rules(tmp_path)
    g.load_rules_checked(rules.path, True)
    assert g.load_bytes(b'not a save') == Status.IO
    g.close()


def test_panels_render(game):
    buf = bytearray(640 * 480 * 3)
    items, n = ac.render_panel('research', game.research_panel(), buf, 640, 480, (0, 0, 560, 420))
    assert n == len(items) and items[0][5] == 'RESEARCH' and any(buf)
    sp = game.social_panel()
    assert sp.valid and sp.faction == 1
    dp = game.design_panel()
    assert dp.counts[0] >= 1
