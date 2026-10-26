import subprocess, os, json, datetime, hashlib, shlex
from pathlib import Path
root=Path('/home/steven-studio/work/systolic-mlir')
out=Path(__file__).resolve().parent
cfg=Path('/home/steven-studio/work/vivado/tile_pipelined/systolic_dma_v1_k8_16_k4_16_1x8x8_3x4x4_shared_wb_r2_tclfix_1025/systolic_dma.runs/impl_1/fpga_config.txt')
env={'HOME':os.environ['HOME'],'USER':os.environ.get('USER','steven-studio'),'PATH':'/usr/local/bin:/usr/bin:/bin','SYSTOLIC_FPGA_CONFIG':str(cfg)}
plan=[('quick-sync',['--quick','--mode','sync']),('regression-sync',['--regression','--mode','sync']),('full-sync',['--mode','sync']),('regression-async',['--regression','--mode','async'])]
records=[]
for name,args in plan:
 cmd=['bash',str(root/'test/e2e/run_e2e.sh'),'--backend','board','--build-dir',str(out/'compiler-build'),*args,'--out',str(out/('fresh-'+name))]
 record={'suite':name,'command':shlex.join(cmd),'environment':env,'started':datetime.datetime.now().astimezone().isoformat(),'git_commit':subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip()}
 print('執行：'+name,flush=True)
 with (out/('fresh-'+name+'.log')).open('x') as log:
  p=subprocess.run(cmd,cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT)
 record.update(returncode=p.returncode,finished=datetime.datetime.now().astimezone().isoformat())
 records.append(record)
 (out/'fresh-executions.json').write_text(json.dumps(records,indent=2,ensure_ascii=False)+'\n')
 print(name+'：結束碼 '+str(p.returncode),flush=True)
 if p.returncode:
  print('測試失敗；停止後續測試與成功封存。',flush=True)
  raise SystemExit(p.returncode)
