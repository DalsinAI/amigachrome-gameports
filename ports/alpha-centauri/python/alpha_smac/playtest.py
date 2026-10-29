# SPDX-License-Identifier: GPL-3.0-or-later
"""The native scripted play test (native/play_control.cpp, run by
native/test_play_session.py) played through the Python API.

    python -m alpha_smac.playtest RULES_DIR EXPANSION SEED OUT_DIR [TURNS]

Every step, decision and printed line follows play_control.cpp's
play_game() one for one, so the output text and the two saves
(OUT_DIR/play.SAV, OUT_DIR/final.SAV) must be byte-identical to a native
host run with the same arguments. The output is written to stdout and to
OUT_DIR/stdout.txt (cp1252 bytes, like printf).
"""
import ctypes as C
import sys
from pathlib import Path

from . import _lib as L
from .game import Cmd, Game, Item, Key, Status, World, command_name, diplomacy_available, fnv1a, new_game_available, \
    render_panel, panel_key, status_name, text, lib

FW, FH = 640, 480
DESIGN_WEAPON = 1


class PlayTest:
    def __init__(self, out=None):
        self.lines = []
        self.failures = 0
        self.out = out
        self.frame = bytearray(FW * FH * 3)

    # ---- output ----
    def p(self, line):
        self.lines.append(line)
        if self.out:
            self.out.write((line + '\n').encode('cp1252', 'replace'))
            self.out.flush()

    def check(self, ok, what):
        if not ok:
            self.failures += 1
            self.p(f'FAIL {what}')

    def ui_frame(self):
        self.frame[:] = bytes(len(self.frame))

    def show(self, panel, kind, model, rect):
        self.ui_frame()
        items, n = render_panel(kind, model, self.frame, FW, FH, rect)
        self.p(f'PANEL {panel} items={n} frame={fnv1a(self.frame):08x}')
        for it in items:
            self.p(f'  |{it[5]}|')

    def drain(self, s, who):
        for m in s.messages():
            self.p(f'{who} MSG t{m.turn} k{m.kind} {m.text}')

    def run(self, s, kind, a=0, b=0, c=0, d=0, e=0, who='A'):
        r = s.run(kind, a, b, c, d, e)
        self.p(f'{who} CMD {command_name(kind)} {a} {b} {c} {d} {e} -> {status_name(r)}')
        self.drain(s, who)
        return r

    def view_check(self, s, who):
        r, data = s.view_bytes()
        self.check(r == Status.OK, 'view bytes')
        w = World(data, 0x12345678)
        self.check(w.ok, 'OriginalWorld load of engine bytes')
        if not w.ok:
            self.p(f'{who} VIEW error={w.error_name}')
            return
        info = w.info()
        self.check(info.vehicle_count == s.vehicle_count, 'view vehicle count')
        self.check(info.base_count == s.base_count, 'view base count')
        mismatched = 0
        for v in range(s.vehicle_count):
            u = s.unit_raw(v)
            o = w.vehicle(v)
            if not o or o[0] != u.x or o[1] != u.y or o[2] != u.unit_id:
                mismatched += 1
        self.check(not mismatched, 'view vehicle positions')
        self.p(f'{who} VIEW bytes={len(data)} digest={fnv1a(data):08x} vehicles={info.vehicle_count} bases={info.base_count} '
               f'turn={info.current_turn}')
        if info.base_count:
            for b in range(info.base_count):
                if w.base_faction(b) == s.player:
                    self.ui_frame()
                    n = w.render_base_window(b, s.player, self.frame, FW, FH, (0, 0, 560, 420))
                    if n >= 0:
                        self.p(f'{who} BASEWIN {b} items={n} frame={fnv1a(self.frame):08x}')
                    break
        w.close()

    def list_units(self, s, who):
        for v in range(s.vehicle_count):
            u = s.unit(v)
            if not u.own:
                continue
            self.p(f'{who} UNIT {v} {u.name} at {u.x},{u.y} moves {u.moves_left}/{u.speed} order {u.order}'
                   f'{" colony" if u.colony else ""}{" former" if u.former else ""}{" combat" if u.combat else ""}')

    @staticmethod
    def find_unit(s, colony, former, combat, skip=-1):
        for v in range(s.vehicle_count):
            u = s.unit_raw(v)
            if not u.own or v == skip:
                continue
            if (colony and u.colony) or (former and u.former) or (combat and u.combat and not u.colony and not u.former):
                return v
        return -1

    @staticmethod
    def own_base(s):
        for i in range(s.base_count):
            m = L.Production()
            if s.production_panel_into(i, m):
                return i
        return -1

    def wander(self, s, v, steps, start_dir):
        for _ in range(steps):
            moved = False
            for d in range(8):
                if moved:
                    break
                direction = (start_dir + d) & 7
                ok, x, y = s.step_target(v, direction)
                if not ok or s.hostile_at(x, y):
                    continue
                before = s.unit_raw(v)
                if before.moves_left <= 0:
                    return
                if self.run(s, Cmd.MOVE, v, direction) == Status.OK:
                    after = s.unit_raw(v)
                    moved = after.x != before.x or after.y != before.y
            if not moved:
                return

    def read_rules(self, s, rules, exp):
        r = s.load_rules(rules, exp)
        self.p(f'RULES {"ok" if r == Status.OK else s.rules_error}')
        return r == Status.OK

    # ---- play_game() of play_control.cpp ----
    def play_game(self, rules, exp, seed, out, turns):
        s = Game()
        if not self.read_rules(s, rules, exp):
            return 20
        if not new_game_available():
            self.p('NEWGAME unavailable in this build')
            return 20
        ng = s.start(seed, 1, 2, 2)
        self.p(f'NEWGAME {status_name(ng)} player={s.player} turn={s.turn} year={s.year}')
        self.drain(s, 'A')
        self.check(ng == Status.OK, 'new game')
        if ng != Status.OK:
            return 20
        self.view_check(s, 'A')
        self.list_units(s, 'A')

        base = self.own_base(s)
        self.check(base >= 0, 'own base')
        self.p(f'BASES {s.base_count}')
        prod = L.Production()
        if base >= 0 and s.production_panel_into(base, prod):
            facility, colony = -1, -1
            for i in range(prod.count):
                it = prod.items[i]
                if it.kind == Item.FACILITY and facility < 0:
                    facility = it.id
                if it.kind == Item.COLONY and colony < 0:
                    colony = it.id
            self.check(colony >= 0, 'colony pod buildable')
            self.check(self.run(s, Cmd.PRODUCE, base, colony if colony >= 0 else prod.items[0].id) == Status.OK, 'set production')
            if facility < 0:
                facility = prod.items[0].id
            self.check(self.run(s, Cmd.QUEUE_ADD, base, facility) == Status.OK, 'queue facility')
            s.production_panel_into(base, prod)
            self.show('production', 'production', prod, (0, 0, 620, 420))
            key = panel_key('production', prod, ord('2'))
            self.p(f'BASEKEY down -> {key} selected={prod.selected}')
            self.run(s, Cmd.HURRY, base)
        former = self.find_unit(s, False, True, False)
        if former >= 0:
            self.run(s, Cmd.TERRAFORM, former, 5)
        scout = self.find_unit(s, False, False, True)
        founded = 0
        for t in range(turns):
            pod = self.find_unit(s, True, False, False)
            if pod >= 0:
                self.wander(s, pod, 2, 2 * t + 1)
                if self.run(s, Cmd.BUILD_BASE, pod) == Status.OK:
                    founded += 1
            if scout >= 0:
                u = s.unit_raw(scout)
                if u is not None and u.own and u.combat:
                    self.wander(s, scout, 2, t)
                else:
                    scout = self.find_unit(s, False, False, True)
            if former >= 0:
                u = s.unit_raw(former)
                if u is not None and u.own and u.former and not u.order:
                    self.run(s, Cmd.TERRAFORM, former, 0 if t & 1 else 5)
            r = self.run(s, Cmd.END_TURN)
            ti = s.last_turn()
            self.p(f'A TURN status={ti.status} faction={ti.faction} turn={ti.turn} year={ti.year} events={ti.events} '
                   f'credits={s.credits} units={s.vehicle_count} bases={s.base_count}')
            self.check(r in (Status.OK, Status.GAME_OVER), 'end turn')
            if r == Status.GAME_OVER:
                break
            self.view_check(s, 'A')
        self.p(f'FOUNDED {founded}')
        self.check(founded > 0, 'base founded by a colony pod')
        self.list_units(s, 'A')

        attacker = self.find_unit(s, False, False, True)
        self.check(attacker >= 0, 'combat unit for the attack')
        if attacker >= 0:
            a = s.unit_raw(attacker)
            enemy_faction = next((f for f in range(1, 8) if f != s.player), -1)
            tx = ty = -1
            for d in range(8):
                if tx >= 0:
                    break
                ok, x, y = s.step_target(attacker, d)
                if ok and s.unit_at(x, y) < 0 and s.base_at(x, y) < 0:
                    if self.run(s, Cmd.EDIT_UNIT, enemy_faction, a.unit_id, x, y) == Status.OK:
                        tx, ty = x, y
            self.check(tx >= 0, 'enemy placed')
            if tx >= 0:
                self.run(s, Cmd.ODDS, attacker, tx, ty)
                self.show('odds', 'battle', s.battle_panel(), (0, 0, 520, 300))
                self.p(f'BATTLEKEY enter -> {panel_key("battle", s.battle_panel(), Key.ENTER)}')
                self.run(s, Cmd.MOVE, attacker, 0)
                r = self.run(s, Cmd.ATTACK, attacker, tx, ty)
                self.check(r == Status.OK, 'attack')
                self.show('battle', 'battle', s.battle_panel(), (0, 0, 520, 300))
            self.view_check(s, 'A')

        rp = L.Research()
        self.check(s.research_panel(rp) is not None, 'research model')
        self.show('research', 'research', rp, (0, 0, 560, 420))
        if rp.count > 1:
            panel_key('research', rp, Key.DOWN)
            r = self.run(s, Cmd.RESEARCH, rp.ids[rp.selected])
            if r == Status.NEEDS_CONFIRM:
                r = self.run(s, Cmd.RESEARCH, rp.ids[rp.selected], 0, 1)
            self.check(r == Status.OK, 'research change')
            s.research_panel(rp)
            self.check(rp.current == rp.ids[1] or rp.current == rp.ids[rp.selected], 'research goal set')
        sp = L.Social()
        self.check(s.social_panel(sp) is not None, 'social model')
        self.show('social', 'social', sp, (0, 0, 620, 320))
        done = False
        for c in range(4):
            for k in range(4):
                if sp.available[c][k] and sp.current[c] != k and sp.cost[c][k] <= sp.credits:
                    sp.row, sp.col = c, k
                    done = True
                    break
            if done:
                break
        if panel_key('social', sp, Key.ENTER) == Key.A_APPLY:
            self.run(s, Cmd.SOCIAL, sp.row, sp.col)
        dp = L.Design()
        self.check(s.design_panel(dp) is not None, 'design model')
        dp.row = DESIGN_WEAPON
        panel_key('design', dp, Key.RIGHT)
        s.design_preview(dp)
        self.show('design', 'design', dp, (0, 0, 560, 360))
        if dp.preview_ok:
            c, w, ar, rr, ab = Game.design_selection(dp)
            self.run(s, Cmd.DESIGN, c, w, ar, rr, ab)
        if base >= 0:
            self.run(s, Cmd.HURRY, base)
        ids = s.contacts()
        self.p(f'CONTACTS {len(ids)} diplomacy={int(diplomacy_available())}')
        if ids and diplomacy_available():
            dm = L.Diplo()
            if s.diplomacy_panel(ids[0], dm) is not None:
                self.show('diplo', 'diplo', dm, (0, 0, 560, 400))
                if dm.count:
                    self.run(s, Cmd.DIPLO, ids[0], dm.proposals[0])
        self.run(s, Cmd.END_TURN)
        self.view_check(s, 'A')

        r, saved = s.save_bytes()
        self.check(r == Status.OK, 'save')
        path = Path(out) / 'play.SAV'
        path.write_bytes(saved)
        self.p(f'SAVE bytes={len(saved)} digest={fnv1a(saved):08x}')
        again = path.read_bytes()
        self.check(again == saved, 'save file bytes')
        b, c = Game(), Game()
        self.check(self.read_rules(b, rules, exp), 'rules B')
        self.check(self.read_rules(c, rules, exp), 'rules C')
        self.check(b.load_bytes(again) == Status.OK, 'load B')
        self.drain(b, 'B')
        self.check(c.load_bytes(again) == Status.OK, 'load C')
        self.drain(c, 'C')
        self.check(b.digest() == s.digest(), 'loaded state equals saved state')
        for t in range(3):
            self.run(s, Cmd.END_TURN, who='A')
            self.run(b, Cmd.END_TURN, who='B')
            self.run(c, Cmd.END_TURN, who='C')
            db, dc = b.digest(), c.digest()
            self.p(f'CONTINUE {t} A={s.digest():08x} B={db:08x} C={dc:08x}')
            self.check(db == dc, 'continued loaded games equal')
        if b.base_count:
            pb = L.Production()
            for i in range(b.base_count):
                if b.production_panel_into(i, pb):
                    break
            if pb.valid and pb.count:
                self.run(b, Cmd.PRODUCE, pb.base, pb.items[pb.count - 1].id, who='B')
        self.run(b, Cmd.END_TURN, who='B')
        self.view_check(b, 'B')
        r, fin = b.save_bytes()
        (Path(out) / 'final.SAV').write_bytes(fin)
        self.p(f'FINAL turn={b.turn} units={b.vehicle_count} bases={b.base_count} digest={b.digest():08x} failures={self.failures}')
        c.close()
        b.close()
        s.close()
        return 1 if self.failures else 0


def main(argv=None):
    argv = sys.argv[1:] if argv is None else argv
    if len(argv) < 4:
        print('usage: python -m alpha_smac.playtest RULES_DIR EXPANSION SEED OUT_DIR [TURNS]')
        return 20
    rules, exp, seed, out = argv[0], int(argv[1]), int(argv[2]) & 0xFFFFFFFF, Path(argv[3])
    turns = int(argv[4]) if len(argv) > 4 else 20
    out.mkdir(parents=True, exist_ok=True)
    with open(out / 'stdout.txt', 'wb') as log:
        class Tee:
            def write(self, b):
                log.write(b)
                sys.stdout.buffer.write(b)

            def flush(self):
                log.flush()
                sys.stdout.buffer.flush()
        t = PlayTest(Tee())
        return t.play_game(rules, exp, seed, str(out), turns)


if __name__ == '__main__':
    sys.exit(main())
