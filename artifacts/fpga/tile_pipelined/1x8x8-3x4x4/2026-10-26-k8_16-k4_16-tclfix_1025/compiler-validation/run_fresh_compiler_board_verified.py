from pathlib import Path
import subprocess, os, json, datetime, shlex,re
root=Path('/home/steven-studio/work/systolic-mlir'); out=Path(__file__).resolve().parent; build=out/'compiler-build'
cfg='/home/steven-studio/work/vivado/tile_pipelined/systolic_dma_v1_k8_16_k4_16_1x8x8_3x4x4_shared_wb_r2_tclfix_1025/systolic_dma.runs/impl_1/fpga_config.txt'
env={'HOME':os.environ['HOME'],'USER':os.environ.get('USER','steven-studio'),'PATH':str(build/'bin')+':/usr/local/bin:/usr/bin:/bin','SYSTOLIC_FPGA_CONFIG':cfg,'LLVM_BUILD':'/home/steven-studio/work/llvm-project/build','SYSTOLIC_CFRONT_CLANG':'/home/steven-studio/work/clang24-build/bin/clang','SYSTOLIC_RUNTIME_TRACE':'1','SYSTOLIC_RUNTIME_TIMING':'1','SYSTOLIC_DPTI_TRACE':'1'}
records=[]
def run(cmd,directory,label):
 with (directory/(label+'.log')).open('x') as f:
  f.write('執行指令：'+shlex.join(list(map(str,cmd)))+'\n');f.flush()
  p=subprocess.run(list(map(str,cmd)),cwd=root,env=env,stdout=f,stderr=subprocess.STDOUT)
 if p.returncode: raise RuntimeError(label+' 結束碼 '+str(p.returncode))
 return (directory/(label+'.log')).read_text()
for source,mode in [('test/reproducibility/gemm_example.c','sync'),('test/reproducibility/gemm_example.c','async'),('test/SystolicCC/affine_mapping.c','async'),('test/SystolicCC/affine_batched.c','async')]:
 name=Path(source).stem+'-'+mode; d=out/'fresh-c-board-verified'/name;d.mkdir(parents=True,exist_ok=False)
 print('新編譯器實板驗證：'+name,flush=True)
 record={'source':source,'submission':mode,'started':datetime.datetime.now().astimezone().isoformat()}
 cmd=[build/'bin/systolic-cc','--build-dir',build,'--llvm-build',env['LLVM_BUILD'],'--clang',env['SYSTOLIC_CFRONT_CLANG'],'--runtime=board','--fp-reassociation','--submission',mode,'--explain','--verify','--keep-dir',d/'ir',root/source,'-o',d/'program']
 run(cmd,d,'compile');run(['ldd',d/'program'],d,'ldd')
 softcmd=cmd.copy(); softcmd[softcmd.index('--runtime=board')]='--runtime=soft'; softcmd[softcmd.index('--keep-dir')+1]=d/'soft-ir'; softcmd[-1]=d/'soft-program'
 run(softcmd,d,'soft-label-compile')
 with (d/'soft-label-reference.log').open('x') as f:
  soft=subprocess.run([str(d/'soft-program')],cwd=root,env={**env,'SOFT_RUNTIME_MOCK_HARDWARE':'1'},stdout=f,stderr=subprocess.STDOUT)
 assert soft.returncode==0
 labels=lambda text: re.findall(r'^\[systolic-cc\] (?:FPGA|CPU) Executed: GEMM #\d+',text,re.M)
 expected=labels((d/'soft-label-reference.log').read_text())
 text=run([d/'program'],d,'execution')
 fpga=len(re.findall(r'^\[systolic-cc\] FPGA Executed:',text,re.M)); cpu=len(re.findall(r'^\[systolic-cc\] CPU Executed:',text,re.M))
 assert fpga>0 and labels(text)==expected and '[libftdi_cosim]' not in text,(name,fpga,cpu,labels(text),expected)
 assert 'mismatch' not in text.lower() or not re.search(r'mismatch(?:es)?\s*[=:]\s*[1-9]',text,re.I)
 record.update(finished=datetime.datetime.now().astimezone().isoformat(),result='PASS',fpga_executed=fpga,cpu_reference_or_planned_host=cpu,cpu_fallback=0)
 records.append(record);(out/'fresh-c-board-verified/results.json').write_text(json.dumps(records,indent=2,ensure_ascii=False)+'\n')
 print(name+' PASS：FPGA '+str(fpga)+'，計畫內 CPU '+str(cpu)+'，CPU fallback 0',flush=True)
