"""Owned local browser build. Never edits firmware, Root, PUB or deployment."""
from pathlib import Path
import argparse,json,hashlib,shutil,subprocess,os,re,time
D=Path(__file__).resolve().parent
parser=argparse.ArgumentParser(description='Build the actual accepted C++firmware as a static browser demo.')
parser.add_argument('--source',type=Path,required=True,help='Accepted firmware/src directory')
parser.add_argument('--assets',type=Path,required=True,help='Runtime firmware/data directory')
parser.add_argument('--out',type=Path,default=D/'dist')
parser.add_argument('--empp',default=shutil.which('em++'),help='Emscripten em++ executable or em++.py; source emsdk_env first')
args=parser.parse_args()
assert args.empp,'Emscripten em++ is required; use the pinned SDK described in README.'
original=args.source.resolve();source=D/'build/source';dist=args.out.resolve();dist.mkdir(parents=True,exist_ok=True)
assert original != source.resolve() and source.resolve() not in original.parents,'Input must not be the generated source snapshot'
if source.exists():shutil.rmtree(source)
source.mkdir(parents=True)
H=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
identity=(original/'math_runtime_identity.h').read_text();table=(original/'static_grand_table.h').read_text()
accepted='accepted=true'in identity and'rootAccepted=true'in table and'hostTrial=false'in table
assert accepted,'Only actually accepted source may create the playable build. No proof-mask or QA override is supplied.'
assert not re.search(r'LocalAnalyticEnclosure,[^3]',table) if accepted else True
pins={p.relative_to(original).as_posix():H(p)for p in original.rglob('*')if p.is_file()and p.suffix in['.h','.hpp','.cpp','.c','.inc']}
runtime=re.search(r'fingerprint="([^"]+)"',identity)[1];family=re.search(r'mathematicalFamily="([^"]+)"',identity)[1]
runtimeSources={name:sha for name,sha in pins.items()if Path(name).name not in ['static_grand_table.h','math_runtime_identity.h']}
assert hashlib.sha256(json.dumps({'family':family,'sources':runtimeSources},sort_keys=True,separators=(',',':')).encode()).hexdigest()==runtime,'Source identity must bind every actual firmware runtime source'
for name in pins:
 out=source/name;out.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(original/name,out)
for name,sha in pins.items():assert H(source/name)==sha and H(original/name)==sha,name
# Runtime files only: never package research, source footage, user pictures or banks.
assetSource=args.assets.resolve();assets={}
for name in ['feature_music.mp3','phoenix_atlas.zlib']:
 p=assetSource/name;assert p.is_file();assets[name]={'sha256':H(p),'bytes':p.stat().st_size}
expected=bytes(int(v,16)for v in re.findall(r'0x([0-9a-f]{2})',(source/'music_asset.h').read_text()))
assert hashlib.sha256((assetSource/'feature_music.mp3').read_bytes()).digest()==expected
atlas=(assetSource/'phoenix_atlas.zlib').read_bytes();fnv=2166136261
for value in atlas:fnv=((fnv^value)*16777619)&0xffffffff
atlasHeader=(source/'phoenix_asset.h').read_text();assert len(atlas)==int(re.search(r'compressedBytes=(\d+)',atlasHeader)[1])and fnv==int(re.search(r'compressedFNV=(\d+)',atlasHeader)[1])
empp=Path(args.empp).resolve();compiler=['python3',str(empp)]if empp.suffix=='.py'else[str(empp)]
versionCheck=subprocess.run(compiler+['--version'],capture_output=True,text=True,check=True)
toolchain={'version':re.search(r'emcc.*? (\d+\.\d+\.\d+)',versionCheck.stdout)[1]}
assert toolchain['version']=='7.0.0','Use pinned Emscripten 7.0.0 for this browser port'

units=['ui','entry_art','collect_art','fire_field','counter_hud','generated_assets','phoenix_art','phoenix_effects','clean_coins','coin_sampler','winged_phoenix','award_art','pdf_assets','phoenix_numerals','audio','audio_cues','audio_music','music_stream','twin_music42']
exports=['pl_boot','pl_preferences','pl_tick','pl_frame','pl_input','pl_retry_storage','pl_snapshot','pl_info','pl_game_view','pl_return_input_fenced','pl_pixels','pl_checkpoint_data','pl_checkpoint_size','pl_checkpoint_capacity','malloc','free']
cmd=compiler+['-std=c++17','-O3','-g0','-DARDUINO','-I',str(D/'platform'),'-I',str(source),'-ffile-prefix-map='+str(D)+'=BROWSER','-fmacro-prefix-map='+str(D)+'=BROWSER',str(D/'bridge.cpp'),*[str(source/(n+'.cpp'))for n in units],'-sUSE_ZLIB=1','-sASYNCIFY=1','-sASYNCIFY_STACK_SIZE=262144','-sSTACK_SIZE=1048576','-sINITIAL_MEMORY=67108864','-sALLOW_MEMORY_GROWTH=1','-sMODULARIZE=1','-sEXPORT_NAME=PhoenixModule','-sENVIRONMENT=web,worker,node','-sEXPORTED_FUNCTIONS='+json.dumps(['_'+n for n in exports]),'-sEXPORTED_RUNTIME_METHODS='+json.dumps(['ccall','UTF8ToString','FS']),'-sASSERTIONS=1','-sFILESYSTEM=1','--no-entry']
for name in assets:cmd+=['--preload-file',str(assetSource/name)+'@/firmware/data/'+name]
cmd+=['-o','phoenix.js']
env=dict(os.environ);env.setdefault('EMCC_CORES','4')
(D/'BUILD_COMMAND.json').write_text(json.dumps(cmd,indent=2)+'\n');began=time.time()
with(D/'BUILD.log').open('w')as log:
 p=subprocess.run(cmd,cwd=dist,env=env,stdout=log,stderr=subprocess.STDOUT)
assert p.returncode==0,(D/'BUILD.log').read_text()[-5000:]
for name,sha in pins.items():assert H(original/name)==sha,'Input changed during build: '+name
version=re.search(r'#define PHOENIX_BUILD_VERSION "([^"]+)"',(source/'build_version.h').read_text())[1]
runtime=re.search(r'fingerprint="([^"]+)"',identity)[1];family=re.search(r'mathematicalFamily="([^"]+)"',identity)[1]
for p in(D/'site').iterdir():
 if p.is_file():shutil.copyfile(p,dist/p.name)
holdMs=int(re.search(r'kHoldMs\s*=\s*(\d+)',(source/'main.cpp').read_text())[1]);doubleMs=int(re.search(r'windowMs\s*=\s*(\d+)',(source/'game_black_tap.h').read_text())[1])
(dist/'control-config.js').write_text('export const CONTROL=Object.freeze('+json.dumps({'holdMs':holdMs,'doubleMs':doubleMs})+');\n')
adapterFiles=[D/'bridge.cpp',*sorted((D/'platform').rglob('*.h')),*sorted((D/'site').glob('*'))]
adapterPins={p.relative_to(D).as_posix():H(p)for p in adapterFiles if p.is_file()}
adapterSha=hashlib.sha256(json.dumps(adapterPins,sort_keys=True,separators=(',',':')).encode()).hexdigest()
public={'version':version,'runtime':runtime,'runtimeMeaning':'Accepted firmware source fingerprint; browser adapter is separately bound','mathFamily':family,'acceptedSource':accepted,'EngineSHA256':pins['phoenix_engine.h'],'UISHA256':pins['ui.cpp'],'tableSHA256':pins['static_grand_table.h'],'identitySHA256':pins['math_runtime_identity.h'],'browserAdapterSHA256':adapterSha,'adapter':'C++ browser platform; noUI_HOST_TEST orPHOENIX_ENGINE_TESTING','gameRules':'Actual local C++ model; no casino-PAR or calibrated-odds claim','assets':assets,'persistence':'Canonical C++checkpoint; awaitedIndexedDBtransaction andbyte-for-bytereadback beforeoutcomeframe','audio':'OriginalC++PCM/MP3decoder+worker;WebAudiooutputdiffersfromphysicalspeaker/DMA','build':'Emscripten'+toolchain['version']}
(dist/'PROVENANCE.json').write_text(json.dumps(public,indent=2)+'\n')
webHash=hashlib.sha256(b''.join(p.name.encode()+hashlib.sha256(p.read_bytes()).digest()for p in sorted(dist.iterdir())if p.is_file()and p.name!='sw.js')).hexdigest()
cache=webHash[:20]
(dist/'sw.js').write_text("const PREFIX='phoenix-cpp-demo:'+self.registration.scope+':';const CACHE=PREFIX+"+json.dumps(cache)+";const FILES=['./','./index.html','./app.js','./style.css','./control-config.js','./phoenix.js','./phoenix.wasm','./phoenix.data','./PROVENANCE.json','./THIRD_PARTY_LICENSES.txt'];self.addEventListener('install',e=>e.waitUntil(caches.open(CACHE).then(c=>c.addAll(FILES))));self.addEventListener('activate',e=>e.waitUntil(caches.keys().then(keys=>Promise.all(keys.filter(k=>k.startsWith(PREFIX)&&k!==CACHE).map(k=>caches.delete(k))))));self.addEventListener('fetch',e=>{if(e.request.method==='GET'&&new URL(e.request.url).origin===location.origin)e.respondWith(caches.match(e.request).then(r=>r||fetch(e.request)));});\n")
for p in dist.iterdir():
 if p.is_file():
  data=p.read_bytes();assert str(Path.home()).encode()not in data and b'/var/folders/'not in data,'Private path in '+p.name
record={'status':'PASS_LOCAL_WASM_BUILD_NOT_PUBLISHED','source':str(original),'sourcePins':pins,'adapterPins':adapterPins,'sourceAccepted':accepted,'runtime':runtime,'buildSeconds':time.time()-began,'artifacts':{p.name:{'bytes':p.stat().st_size,'sha256':H(p)}for p in dist.iterdir()if p.is_file()},'output':str(dist),'publicProvenance':public,'no_firmware_source_edits':True,'no_GitHub_Sites_PUB_device_writes':True}
(D/'BUILD_RESULT.json').write_text(json.dumps(record,indent=2)+'\n');print(json.dumps({k:v for k,v in record.items()if k not in ['sourcePins','artifacts','publicProvenance']},indent=2),flush=True)
