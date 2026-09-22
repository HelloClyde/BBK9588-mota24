"""Exercise a normal first-floor battle on a dedicated emulator at the title screen.
Does not write saves. --launch navigates the emulator launcher first.
"""
import argparse,io,json,re,time
from pathlib import Path
import requests
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
ap=argparse.ArgumentParser();ap.add_argument('--base',required=True);ap.add_argument('--launch',action='store_true');a=ap.parse_args()
OUT=ROOT/'build/qa';OUT.mkdir(parents=True,exist_ok=True)
def req(path,post=False,**params):
 r=(requests.post if post else requests.get)(a.base+path,params=params,timeout=30);r.raise_for_status()
 if 'json' in r.headers.get('content-type',''):assert not r.json().get('error'),r.text
 return r
def key(code,n=1):
 for _ in range(n):
  for down in (1,0):req('/api/key',True,code=code,down=down);time.sleep(.2)
def touch(x,y):
 for down in (1,0):req('/api/touch',True,x=x,y=y,down=down);time.sleep(.25)
def screen():return Image.open(io.BytesIO(req('/screen.png').content)).convert('RGB')
if a.launch:touch(198,84);time.sleep(2);touch(120,100);time.sleep(2)
key(10);key(9);key(4);key(10,16);key(4,9)
key(4,2);key(7,5);key(4,7);key(6,4)
before=screen();before.save(OUT/'battle-before.png')
req('/api/key',True,code=6,down=1);time.sleep(.04);req('/api/key',True,code=6,down=0)
frames=[]
for i in range(40):
 frames.append(screen());time.sleep(.08)
frames[-1].save(OUT/'battle-after.png')
frames[0].save(OUT/'battle-process.gif',save_all=True,append_images=frames[1:],duration=100,loop=0)
# Compare the displayed final 950 HP against the game's own numeric glyphs.
s=(ROOT/'src/font.h').read_text(encoding='utf8');expected=[]
rows=[list(map(int,re.search(r'\{'+str(ord(c))+r',\{([^}]+)\}',s).group(1).split(','))) for c in '950']
for y in range(12):
 for r in rows:
  expected.extend(bool(r[y]&(1<<(11-x))) for x in range(6))
actual=[all(c>200 for c in px) for px in frames[-1].crop((150,4,168,16)).getdata()]
assert actual==expected,'Final HP does not match 950'
assert len({im.crop((14,66,226,277)).tobytes() for im in frames})>4,'No visible combat phases'
# Red hit frames distinguish battle from the plain dungeon.
hits=[im for im in frames if im.getpixel((164,119))[0]>150 and im.getpixel((164,119))[1]<130]
assert hits,'No hero-hit frame captured';hits[0].save(OUT/'battle-hit.png')
key(10);key(5,4);key(10) # Restore the user's previous save without writing test progress.
(OUT/'battle-result.json').write_text(json.dumps({'passed':True,'base':a.base,'route':'normal first-floor green slime','starting_hp':1000,'ending_hp':950,'captured_frames':len(frames)},indent=2))
print('PASS emulator combat animation: visible attack phases and exact final 950 HP')
