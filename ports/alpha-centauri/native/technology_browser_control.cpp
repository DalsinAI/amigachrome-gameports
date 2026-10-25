/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "technology_browser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int checks=0;
#define CHECK(x) do{++checks;if(!(x)){printf("FAIL %d line %d\n",checks,__LINE__);return 20;}}while(0)
int main(int argc,char** argv){
 alpha::Rules rules{};rules.count=alpha::technology_count;
 for(size_t i=0;i<rules.count;++i)rules.technologies[i].prerequisites[0]=rules.technologies[i].prerequisites[1]=-2;
 rules.technologies[0].prerequisites[0]=rules.technologies[0].prerequisites[1]=-1;
 rules.technologies[1].prerequisites[0]=0;rules.technologies[1].prerequisites[1]=-1;
 rules.technologies[2].prerequisites[0]=0;rules.technologies[2].prerequisites[1]=1;
 alpha::TechnologyTree tree{};CHECK(tree.build(rules));CHECK(tree.count==3);CHECK(tree.depth[0]==0 && tree.depth[1]==1 && tree.depth[2]==2);CHECK(tree.depth[3]==-1);
 rules.technologies[0].prerequisites[0]=2;alpha::TechnologyTree before=tree;CHECK(!tree.build(rules));CHECK(!memcmp(&before,&tree,sizeof(tree)));
 rules.technologies[0].prerequisites[0]=88;CHECK(!tree.build(rules));rules.technologies[0].prerequisites[0]=89;CHECK(!tree.build(rules));rules.technologies[0].prerequisites[0]=-3;CHECK(!tree.build(rules));rules.count=88;CHECK(!tree.build(rules));
 const char source[]="#TECH1\r\n{One} [two]\r\nnext\r\n#TECH10\nWrong\n";char out[40]="unchanged";
 CHECK(alpha::technology_description((const uint8_t*)source,strlen(source),1,out,sizeof(out)));CHECK(!strcmp(out,"One two next"));CHECK(!alpha::technology_description((const uint8_t*)source,strlen(source),0,out,sizeof(out)));CHECK(!alpha::technology_description((const uint8_t*)source,strlen(source),1,out,3));CHECK(!alpha::technology_description((const uint8_t*)source,sizeof(source),1,out,sizeof(out)));CHECK(!alpha::technology_description(nullptr,0,1,out,sizeof(out)));CHECK(!alpha::technology_description((const uint8_t*)source,strlen(source),-1,out,sizeof(out)));
 if(argc==2){FILE* f=fopen(argv[1],"rb");CHECK(f);CHECK(!fseek(f,0,SEEK_END));long n=ftell(f);CHECK(n>0 && n<2000000);rewind(f);auto* p=(uint8_t*)malloc((size_t)n);CHECK(p);CHECK(fread(p,1,(size_t)n,f)==(size_t)n);fclose(f);CHECK(alpha::read_technologies(p,(size_t)n,rules)==alpha::Error::ok);free(p);CHECK(tree.build(rules));CHECK(tree.count==rules.active);size_t edges=0;for(size_t i=0;i<rules.count;++i)if(tree.depth[i]>=0)for(int k=0;k<2;++k){int prerequisite=rules.technologies[i].prerequisites[k];if(prerequisite>=0){CHECK(tree.depth[prerequisite]<tree.depth[i]);++edges;}}CHECK(edges==rules.edges);printf("ORIGINAL active=%lu edges=%lu\n",(unsigned long)tree.count,(unsigned long)edges);}
 printf("TECHNOLOGY BROWSER %d checks PASS\n",checks);return 0;
}
