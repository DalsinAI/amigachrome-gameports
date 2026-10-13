"""Compile the exact Amiga directory helper carried by the patch, not a rewrite."""
from pathlib import Path
import re,shutil,subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
PATCH=ROOT/'ports/openjazz/patches/0002-amiga-directory-write-check.patch'

class OpenJazzDirectoryTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which('c++'),'host C++ compiler required')
    def test_actual_helper_checks_directory_write_bit(self):
        added='\n'.join(x[1:] for x in PATCH.read_text().splitlines() if x.startswith('+') and not x.startswith('+++'))
        helper=re.search(r'static bool writable_config_directory\(.*?\n\}',added,re.S).group(0)
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);good=root/'good';good.mkdir();readonly=root/'readonly';readonly.mkdir();readonly.chmod(0o500)
            regular=root/'file';regular.write_text('not a directory')
            code=root/'probe.cpp';binary=root/'probe'
            code.write_text('#define __amigaos__ 1\n#include <sys/stat.h>\n#include <unistd.h>\n'+helper+'\nint main(int n,char **p){return n!=5 || !writable_config_directory(p[1]) || writable_config_directory(p[2]) || writable_config_directory(p[3]) || writable_config_directory(p[4]);}\n')
            subprocess.run(['c++','-std=c++11','-Wall','-Wextra','-Werror',str(code),'-o',str(binary)],check=True,capture_output=True)
            subprocess.run([str(binary),str(good),str(readonly),str(regular),str(root/'missing')],check=True)
            readonly.chmod(0o700)
    def test_non_amiga_access_path_and_patch_failure_guard_are_retained(self):
        text=PATCH.read_text();self.assertIn('return access(path, W_OK) == 0;',text)
        self.assertIn('if (!writable_config_directory(newPath))',text)
        recipe=(ROOT/'ports/openjazz/build-amigaos3.sh').read_text()
        self.assertIn('apply --reverse --check',recipe)
        self.assertIn('patch does not match pinned source',recipe)

if __name__=='__main__':unittest.main()
