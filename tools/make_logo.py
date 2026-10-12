"""Level Drop logo (original, code-generated): folder + downward arrow + a GD-style level block. 336x336."""
from PIL import Image, ImageDraw
S=336; K=4; W=S*K
im=Image.new("RGBA",(W,W),(0,0,0,0)); d=ImageDraw.Draw(im)
def r(b): return [v*K for v in b]
# rounded backdrop
d.rounded_rectangle(r([8,8,328,328]),radius=56*K,fill=(24,20,48,255),outline=(255,170,40,255),width=8*K)
# folder (back tab + body)
d.rounded_rectangle(r([52,118,150,152]),radius=12*K,fill=(255,140,30,255))
d.rounded_rectangle(r([52,138,284,272]),radius=18*K,fill=(255,170,40,255))
d.rounded_rectangle(r([52,166,284,272]),radius=18*K,fill=(255,196,70,255))
# GD-style level block inside folder: square with inner outline square
bx,by,bs=190,184,70
d.rectangle(r([bx,by,bx+bs,by+bs]),fill=(40,200,255,255),outline=(10,30,60,255),width=6*K)
d.rectangle(r([bx+18,by+18,bx+bs-18,by+bs-18]),outline=(10,30,60,255),width=5*K)
# downward arrow (dropping into folder)
d.rectangle(r([96,40,136,170]),fill=(80,255,170,255))
d.polygon(r([66,160,166,160,116,232]),fill=(80,255,170,255))
d.line(r([96,40,96,160]),fill=(10,60,40,255),width=0)
im=im.resize((S,S),Image.LANCZOS)
im.save(__import__("os").path.join(__import__("os").path.dirname(__file__),"..","logo.png"))
print("logo.png written")
