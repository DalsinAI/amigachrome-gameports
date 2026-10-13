"""Run the exact methods added by the Amiga single-definition patch.

These host tests check method behaviour, not gameplay or target HUNK execution.
The same source can be compiled as a target diagnostic for the isolated lab.
"""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
PATCH=ROOT/'ports/assaultcube/patches/0003-amiga-stream-single-definition.patch'

def diagnostic_source():
    added='\n'.join(line[1:] for line in PATCH.read_text().splitlines()
                    if line.startswith('+') and not line.startswith('+++'))
    begin='// AC_AMIGA_STREAM_SINGLE_DEFINITION_BEGIN'
    end='// AC_AMIGA_STREAM_SINGLE_DEFINITION_END'
    assert added.count(begin)==added.count(end)==1
    implementation=added.split(begin,1)[1].split(end,1)[0]
    return r'''
#include <cstdio>
#include <cstring>
#ifdef getchar
#undef getchar
#endif
#ifdef putchar
#undef putchar
#endif
typedef unsigned char uchar;
struct stream {
    virtual ~stream() {}
    virtual int read(void *,int) { return 0; }
    virtual int write(const void *,int) { return 0; }
    virtual int getchar();
    virtual bool putchar(int);
    virtual bool putstring(const char *);
};
''' + implementation + r'''
struct MemoryStream : stream {
    uchar buffer[32];
    int pos, size, writes, requested;
    bool fail;
    MemoryStream():pos(0),size(0),writes(0),requested(-1),fail(false) { std::memset(buffer,0,sizeof buffer); }
    int read(void *p,int n) override {
        if(fail || n!=1 || pos>=size) return 0;
        *static_cast<uchar *>(p)=buffer[pos++];return 1;
    }
    int write(const void *p,int n) override {
        ++writes;requested=n;
        if(fail || n<0 || size+n>32) return 0;
        std::memcpy(buffer+size,p,n);size+=n;return n;
    }
};
#define CHECK(x,n) do { if(!(x)) { std::printf("STREAM_CONTRACT_FAIL %d\n",n); return n; } } while(0)
int main() {
    stream empty;
    CHECK(empty.getchar()==-1,1);
    CHECK(!empty.putchar(17),2);
    CHECK(!empty.putstring("nonempty"),3);
    CHECK(empty.putstring(""),4);
    MemoryStream mem; stream *p=&mem;
    CHECK(p->putchar(0xff),5);
    CHECK(p->putchar(0),6);
    CHECK(p->getchar()==255,7);
    CHECK(p->getchar()==0,8);
    CHECK(p->getchar()==-1,9);
    CHECK(p->putchar(0x101),10);
    CHECK(p->getchar()==1,11);
    CHECK(p->putstring("abc"),12);
    CHECK(mem.requested==3,13);
    CHECK(p->getchar()=='a' && p->getchar()=='b' && p->getchar()=='c',14);
    CHECK(p->getchar()==-1,15);
    int size=mem.size;
    CHECK(p->putstring(""),16);
    CHECK(mem.size==size && mem.requested==0,17);
    mem.fail=true;
    CHECK(!p->putchar(90),18);
    CHECK(!p->putstring("def"),19);
    CHECK(p->getchar()==-1,20);
    CHECK(mem.size==size,21);
    std::puts("STREAM_CONTRACT_PASS 21 checks");
    return 0;
}
'''

class StreamPatchTests(unittest.TestCase):
    def test_amiga_only_declarations_preserve_non_amiga_inline_path(self):
        text=PATCH.read_text()
        for declaration in ('virtual int getchar();','virtual bool putchar(int n);','virtual bool putstring(const char *str);'):
            self.assertIn('+'+'    '+declaration,text)
        self.assertGreaterEqual(text.count('+#ifdef __amigaos__'),4)
        self.assertNotIn('-fno-devirtualize',text)
    @unittest.skipUnless(shutil.which('c++'),'host C++ compiler required')
    def test_exact_added_methods_preserve_read_write_contract(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d);src=p/'check.cpp';exe=p/'check'
            src.write_text(diagnostic_source())
            subprocess.run(['c++','-std=c++11','-O2','-Wall','-Wextra',str(src),'-o',str(exe)],check=True,capture_output=True,text=True)
            result=subprocess.run([str(exe)],check=True,capture_output=True,text=True)
            self.assertEqual(result.stdout.strip(),'STREAM_CONTRACT_PASS 21 checks')
    def test_mode_buffer_array_delete_fix_is_narrow(self):
        patch=(ROOT/'ports/assaultcube/patches/0004-match-mode-buffer-array-delete.patch').read_text()
        removed=[l[1:] for l in patch.splitlines() if l.startswith('-') and not l.startswith('---')]
        added=[l[1:] for l in patch.splitlines() if l.startswith('+') and not l.startswith('+++')]
        self.assertEqual(removed,['    delete buf;'])
        self.assertEqual(added,['    delete[] buf;'])
        for name in ('build-amiga-server.sh','build-amiga-client.sh'):
            self.assertIn('0004-match-mode-buffer-array-delete.patch',
                          (ROOT/'ports/assaultcube'/name).read_text())

    def test_both_build_recipes_apply_and_verify_the_patch(self):
        for name in ('build-amiga-server.sh','build-amiga-client.sh'):
            text=(ROOT/'ports/assaultcube'/name).read_text()
            self.assertIn(PATCH.name,text)
            self.assertIn('apply --reverse --check "$STREAM_PATCH"',text)
            self.assertIn('stream patch does not match',text)

if __name__=='__main__':unittest.main()
