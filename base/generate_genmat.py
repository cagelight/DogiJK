#!/bin/python

from PIL import Image
import datetime
import os

do_images = True

colors = {
	"black" : [0.1, 0.1, 0.1],
	"white" : [1, 1, 1],
	"grey" : [0.5, 0.5, 0.5],
	"greyl" : [0.75, 0.75, 0.75],
	"greyd" : [0.25, 0.25, 0.25],
	"red" : [1, 0, 0],
	"redl" : [1, 0.5, 0.5],
	"redd" : [0.5, 0, 0],
	"orange" : [1, 0.5, 0],
	"orangel" : [1, 0.75, 0.5],
	"oranged" : [0.5, 0.25, 0],
	"yellow" : [1, 1, 0],
	"yellowl" : [1, 1, 0.5],
	"yellowd" : [0.5, 0.5, 0],
	"lime" : [0.5, 1, 0],
	"limel" : [0.75, 1, 0.5],
	"limed" : [0.25, 0.5, 0],
	"green" : [0, 1, 0],
	"greenl" : [0.5, 1, 0.5],
	"greend" : [0, 0.5, 0],
	"spring" : [0, 1, 0.5],
	"springl" : [0.5, 1, 0.75],
	"springd" : [0, 0.5, 0.25],
	"cyan" : [0, 1, 1],
	"cyanl" : [0.5, 1, 1],
	"cyand" : [0, 0.5, 0.5],
	"dodger" : [0, 0.5, 1],
	"dodgerl" : [0.5, 0.75, 1],
	"dodgerd" : [0, 0.25, 0.5],
	"blue" : [0, 0, 1],
	"bluel" : [0.5, 0.5, 1],
	"blued" : [0, 0, 0.5],
	"violet" : [0.5, 0, 1],
	"violetl" : [0.75, 0.5, 1],
	"violetd" : [0.25, 0, 0.5],
	"magenta" : [1, 0, 1],
	"magental" : [1, 0.5, 1],
	"magentad" : [0.5, 0, 0.5],
	"hotpink" : [1, 0, 0.5],
	"hotpinkl" : [1, 0.5, 0.75],
	"hotpinkd" : [0.5, 0, 0.25],
	"warm" : [1, 0.7, 0.5],
	"cool" : [0.5, 0.7, 1],
	"umber" : [0.51, 0.4, 0.27],
	"kobicha" : [0.42, 0.27, 0.14],
	"gold" : [1, 0.84, 0],
	"copper": [0.79, 0.43, 0.32],
	"vanta" : [0.0, 0.0, 0.0],
}

sets = {

### NORMAL ###

"""
textures/genmat/metal_{c}
{{
	qer_editorimage textures/editor/metal_{c}
		{{
				map textures/genmat/metal
				rgbGen const ( {r:.6f} {g:.6f} {b:.6f} )
		}}
		{{
				map $lightmap
				blendFunc GL_ZERO GL_SRC_COLOR
		}}
}}
""",
"""
textures/genmat/wood_{c}
{{
	qer_editorimage textures/editor/wood_{c}
		{{
				map textures/genmat/wood
				rgbGen const ( {r:.6f} {g:.6f} {b:.6f} )
		}}
		{{
				map $lightmap
				blendFunc GL_ZERO GL_SRC_COLOR
		}}
}}
""",
"""
textures/genmat/cloth_{c}
{{
	qer_editorimage textures/editor/cloth_{c}
		{{
				map textures/genmat/cloth
				rgbGen const ( {r:.6f} {g:.6f} {b:.6f} )
		}}
		{{
				map $lightmap
				blendFunc GL_ZERO GL_SRC_COLOR
		}}
}}
""",
"""
textures/genmat/stone_{c}
{{
	qer_editorimage textures/editor/stone_{c}
		{{
				map textures/genmat/stone
				rgbGen const ( {r:.6f} {g:.6f} {b:.6f} )
		}}
		{{
				map $lightmap
				blendFunc GL_ZERO GL_SRC_COLOR
		}}
}}
""",
"""
textures/genmat/marble_{c}
{{
	qer_editorimage textures/editor/marble_{c}
		{{
				map textures/genmat/marble
				rgbGen const ( {r:.6f} {g:.6f} {b:.6f} )
		}}
		{{
				map $lightmap
				blendFunc GL_ZERO GL_SRC_COLOR
		}}
}}
""",
"""
textures/genmat/cgrating_{c}
{{
	qer_editorimage textures/editor/cgrating_{c}
	qer_trans 0.75
	surfaceparm trans
	cull none
		{{
				map textures/genmat/cgrating
				rgbGen const ( {r:.6f} {g:.6f} {b:.6f} )
				blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
				alphaFunc GE192
				depthWrite
		}}
		{{
				map $lightmap
				blendFunc GL_ZERO GL_SRC_COLOR
		}}
}}
""",

### NONSOLID ###

"""
textures/genmat/metal_{c}_ns
{{
	qer_editorimage textures/editor/metal_{c}
	surfaceparm nonsolid
		{{
				map textures/genmat/metal
				rgbGen const ( {r:.6f} {g:.6f} {b:.6f} )
		}}
		{{
				map $lightmap
				blendFunc GL_ZERO GL_SRC_COLOR
		}}
}}
""",
"""
textures/genmat/wood_{c}_ns
{{
	qer_editorimage textures/editor/wood_{c}
	surfaceparm nonsolid
		{{
				map textures/genmat/wood
				rgbGen const ( {r:.6f} {g:.6f} {b:.6f} )
		}}
		{{
				map $lightmap
				blendFunc GL_ZERO GL_SRC_COLOR
		}}
}}
""",
"""
textures/genmat/cloth_{c}_ns
{{
	qer_editorimage textures/editor/cloth_{c}
	surfaceparm nonsolid
		{{
				map textures/genmat/cloth
				rgbGen const ( {r:.6f} {g:.6f} {b:.6f} )
		}}
		{{
				map $lightmap
				blendFunc GL_ZERO GL_SRC_COLOR
		}}
}}
""",
"""
textures/genmat/stone_{c}_ns
{{
	qer_editorimage textures/editor/stone_{c}
	surfaceparm nonsolid
		{{
				map textures/genmat/stone
				rgbGen const ( {r:.6f} {g:.6f} {b:.6f} )
		}}
		{{
				map $lightmap
				blendFunc GL_ZERO GL_SRC_COLOR
		}}
}}
""",
"""
textures/genmat/marble_{c}_ns
{{
	qer_editorimage textures/editor/marble_{c}
	surfaceparm nonsolid
		{{
				map textures/genmat/marble
				rgbGen const ( {r:.6f} {g:.6f} {b:.6f} )
		}}
		{{
				map $lightmap
				blendFunc GL_ZERO GL_SRC_COLOR
		}}
}}
""",
"""
textures/genmat/cgrating_{c}_ns
{{
	qer_editorimage textures/editor/cgrating_{c}
	surfaceparm nonsolid
	qer_trans 0.75
	surfaceparm trans
	cull none
		{{
				map textures/genmat/cgrating
				rgbGen const ( {r:.6f} {g:.6f} {b:.6f} )
				blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
				alphaFunc GE192
				depthWrite
		}}
		{{
				map $lightmap
				blendFunc GL_ZERO GL_SRC_COLOR
		}}
}}
""",

### UNLIT ###

"""
textures/genmat/metal_{c}_ul
{{
	qer_editorimage textures/editor/metal_{c}
		{{
				map textures/genmat/metal
				rgbGen const ( {r:.6f} {g:.6f} {b:.6f} )
		}}
}}
""",
"""
textures/genmat/cgrating_{c}_ul
{{
	qer_editorimage textures/editor/cgrating_{c}
	qer_trans 0.75
	surfaceparm trans
	cull none
		{{
				map textures/genmat/cgrating
				rgbGen const ( {r:.6f} {g:.6f} {b:.6f} )
				blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
				alphaFunc GE192
				depthWrite
		}}
}}
"""

}

output = '// this file was automatically generated by generate_genmat.py on {}\n\n'.format(datetime.datetime.now());

templates = [
	{
		'path': 'metal_template.png',
		'name': 'metal_{c}',
	},{
		'path': 'wood_template.png',
		'name': 'wood_{c}',
	},{
		'path': 'cloth_template.png',
		'name': 'cloth_{c}',
	},{
		'path': 'stone_template.png',
		'name': 'stone_{c}',
	},{
		'path': 'marble_template.png',
		'name': 'marble_{c}',
	},{
		'path': 'cgrating_template.png',
		'name': 'cgrating_{c}',
	}
]

def lcol(c):
	return c + ((1 - c) / 2)

def dcol(c):
	return c / 2

if not os.path.exists('./textures/editor'):
		os.makedirs('./textures/editor')
		
if not os.path.exists('./shaders'):
		os.makedirs('./shaders')

print('generating editor images...')
for color in colors:
	r = colors[color][0]
	g = colors[color][1]
	b = colors[color][2]
	print('\t{}'.format(color))
	
	if do_images:
		for template in templates:
			templimg = Image.open('./templates/{}'.format(template['path']))
			data = templimg.load()
			if len(data[0, 0]) == 3:
				for x in range(templimg.width):
					for y in range(templimg.height):
						data[x, y] = (int(r * data[x, y][0]), int(g * data[x, y][1]), int(b * data[x, y][2]))
			else:
				for x in range(templimg.width):
					for y in range(templimg.height):
						data[x, y] = (int(r * data[x, y][0]), int(g * data[x, y][1]), int(b * data[x, y][2]), data[x, y][3])
			obj = {'c':color}
			templimg.save('./textures/editor/{}.png'.format(template['name'].format(**obj)))
			
	output += "\n// ==== {} ====\n".format(color.upper())
	
	for set in sets:
		obj = {
			'c':color, 
			'r':colors[color][0], 'g':colors[color][1], 'b':colors[color][2],
			'rl':lcol(colors[color][0]), 'gl':lcol(colors[color][1]), 'bl':lcol(colors[color][2]),
			'rd':dcol(colors[color][0]), 'gd':dcol(colors[color][1]), 'bd':dcol(colors[color][2])
		}
		output += set.format(**obj)

print('writing shader file...')
f = open('./shaders/genmat.shader', 'w')
f.write(output)
f.close()
print('done')
