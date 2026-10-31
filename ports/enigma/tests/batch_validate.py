#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 AmigaChrome Team
from pathlib import Path
import subprocess,json,xml.etree.ElementTree as E,sys
source=Path(sys.argv[1]);binary=Path(sys.argv[2]);out=Path(sys.argv[3]);out.mkdir(exist_ok=True)
groups={};skipped=[]
for f in (source/'data').rglob('*.xml'):
 try:r=E.parse(f).getroot()
 except E.ParseError:skipped.append(str(f));continue
 name=r.tag.split('}')[-1]
 schema={'level':'level','index':'index','state':'state','score':'score','ratings':'ratings','objects':'objects','preferences':'preferences'}.get(name)
 if schema:groups.setdefault(schema,[]).append(f)
results={}
with (out/'fixtures.log').open('w') as log:
 for schema,files in groups.items():
  failed=0
  for i in range(0,len(files),80):
   r=subprocess.run([str(binary),str(source/'data/schemas'/ (schema+'.xsd')),*map(str,files[i:i+80])],stdout=log,stderr=log)
   if r.returncode:failed+=1
  results[schema]={'files':len(files),'failed_batches':failed}
(out/'fixtures.json').write_text(json.dumps({'groups':results,'malformed_xml_files':skipped},indent=2)+'\n')
print(json.dumps(results),flush=True)
