
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

// HUNK link-once checking rejects differently optimised copies of these
// header-defined virtual methods. Keep one definition without changing the
// read/write contract, optimisation flags or non-Amiga builds.
int stream::getchar()
{
    uchar c;
    return read(&c, 1) == 1 ? c : -1;
}

bool stream::putchar(int n)
{
    uchar c = n;
    return write(&c, 1) == 1;
}

bool stream::putstring(const char *str)
{
    int len = (int)strlen(str);
    return write(str, len) == len;
}

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
