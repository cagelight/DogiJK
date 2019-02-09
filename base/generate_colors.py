#!/bin/python

from PIL import Image
import datetime
import os

colors = {
	"vanta" : [0, 0, 0],
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
	"copper": [0.79, 0.43, 0.32]
}

sets = {
"""
textures/colors/s_{c}
{{
	qer_editorimage	textures/editor/s_{c}
	
	{{
		map $whiteimage
		rgbGen const ( {r:.6f} {g:.6f} {b:.6f} )
	}}
	{{
		map $lightmap
		blendFunc GL_ZERO GL_SRC_COLOR
	}}
}}
""",
"""
textures/colors/n_{c}
{{
	qer_editorimage	textures/editor/n_{c}
	surfaceparm	nonsolid
	
	{{
		map $whiteimage
		rgbGen const ( {r:.6f} {g:.6f} {b:.6f} )
	}}
	{{
		map $lightmap
		blendFunc GL_ZERO GL_SRC_COLOR
	}}
}}
""",
"""
textures/colors/lvd_{c}
{{
	qer_editorimage	textures/editor/lvd_{c}
	q3map_lightImage textures/editor/lightimage_{c}
	q3map_surfacelight	8000
	q3map_lightsubdivide	64
	q3map_nolightmap
	
	{{
		map $whiteimage
		rgbGen const ( {r:.6f} {g:.6f} {b:.6f} )
		glow
	}}
}}
""",
"""
textures/colors/ld_{c}
{{
	qer_editorimage	textures/editor/ld_{c}
	q3map_lightImage textures/editor/lightimage_{c}
	q3map_surfacelight	16000
	q3map_lightsubdivide	64
	q3map_nolightmap
	
	{{
		map $whiteimage
		rgbGen const ( {r:.6f} {g:.6f} {b:.6f} )
		glow
	}}
}}
""",
"""
textures/colors/l_{c}
{{
	qer_editorimage	textures/editor/l_{c}
	q3map_lightImage textures/editor/lightimage_{c}
	q3map_surfacelight	32000
	q3map_lightsubdivide	64
	q3map_nolightmap
	
	{{
		map $whiteimage
		rgbGen const ( {r:.6f} {g:.6f} {b:.6f} )
		glow
	}}
}}
""",
"""
textures/colors/lb_{c}
{{
	qer_editorimage	textures/editor/lb_{c}
	q3map_lightImage textures/editor/lightimage_{c}
	q3map_surfacelight	64000
	q3map_lightsubdivide	64
	q3map_nolightmap
	
	{{
		map $whiteimage
		rgbGen const ( {r:.6f} {g:.6f} {b:.6f} )
		glow
	}}
}}
""",
"""
textures/colors/lvb_{c}
{{
	qer_editorimage	textures/editor/lvb_{c}
	q3map_lightImage textures/editor/lightimage_{c}
	q3map_surfacelight	128000
	q3map_lightsubdivide	64
	q3map_nolightmap
	
	{{
		map $whiteimage
		rgbGen const ( {r:.6f} {g:.6f} {b:.6f} )
		glow
	}}
}}
""",
"""
textures/colors/m_{c}
{{
	qer_editorimage	textures/editor/m_{c}
	q3map_nolightmap
	
	{{
		map $whiteimage
		rgbGen const ( {r:.6f} {g:.6f} {b:.6f} )
	}}
	{{
		map $whiteimage
		rgbGen lightingDiffuse
		blendFunc GL_DST_COLOR GL_ZERO
	}}
	{{
		map $whiteimage
		rgbGen lightingDiffuse
		alphaGen lightingSpecular
		blendFunc GL_SRC_ALPHA GL_ONE
	}}
}}
""",
"""
textures/colors/fb_{c}
{{
	qer_editorimage	textures/editor/fb_{c}
	q3map_nolightmap
	
	{{
		map $whiteimage
		rgbGen const ( {r:.6f} {g:.6f} {b:.6f} )
	}}
}}
""",
"""
textures/colors/c_{c}
{{
	qer_editorimage	textures/editor/c_{c}
	surfaceparm	nonopaque
	surfaceparm	forcefield
	surfaceparm	trans
	q3map_material Glass
	qer_trans	0.5
	
	{{
		map textures/colors/crystal_env
		blendFunc GL_ONE GL_ONE
		rgbGen const ( {rl:.6f} {gl:.6f} {bl:.6f} )
		tcGen environment
	}}
	{{
		map $whiteimage
		rgbGen const ( {rd:.6f} {gd:.6f} {bd:.6f} )
		blendFunc GL_ONE GL_ONE
	}}
	{{
		map $lightmap
		blendFunc GL_DST_COLOR GL_SRC_COLOR
	}}

}}
""",
"""
textures/colors/cu_{c}
{{
	qer_editorimage	textures/editor/cu_{c}
	surfaceparm	nonopaque
	surfaceparm	forcefield
	surfaceparm	trans
	q3map_material Glass
	qer_trans	0.5
	q3map_nolightmap
	
	{{
		map $whiteimage
		rgbGen const ( {r:.6f} {g:.6f} {b:.6f} )
		blendFunc GL_ONE GL_ONE
	}}
	{{
		map textures/colors/crystal_env
		blendFunc GL_ONE GL_ONE
		tcGen environment
	}}
}}
""",
"""
textures/colors/cg_{c}
{{
	qer_editorimage textures/editor/cg_{c}
	q3map_lightImage textures/editor/lightimage_{c}
	surfaceparm	nonopaque
	surfaceparm	forcefield
	surfaceparm	trans
	q3map_material Glass
	qer_trans	0.5
	q3map_surfacelight		8000
	q3map_lightsubdivide	512
	q3map_nolightmap
	
	{{
		map $whiteimage
		alphaGen const 0.75
		rgbGen const ( {r:.6f} {g:.6f} {b:.6f} )
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		glow
	}}
	{{
		map textures/colors/crystal_env
		alphaGen const 0.75
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen const ( {rl:.6f} {gl:.6f} {bl:.6f} )
		tcGen environment
	}}
}}
""",
"""
textures/colors/cgl_{c}
{{
	qer_editorimage textures/editor/cgl_{c}
	q3map_lightImage textures/editor/lightimage_{c}
	surfaceparm	nonopaque
	surfaceparm	forcefield
	surfaceparm	trans
	q3map_material Glass
	qer_trans	0.5
	q3map_surfacelight		32000
	q3map_lightsubdivide	512
	q3map_nolightmap
	
	{{
		map $whiteimage
		alphaGen const 0.75
		rgbGen const ( {r:.6f} {g:.6f} {b:.6f} )
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		glow
	}}
	{{
		map textures/colors/crystal_env
		alphaGen const 0.75
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen const ( {rl:.6f} {gl:.6f} {bl:.6f} )
		tcGen environment
	}}
}}
""",
"""
textures/colors/g_{c}
{{
	qer_editorimage textures/editor/g_{c}
	surfaceparm	nonopaque
	surfaceparm	forcefield
	surfaceparm	trans
	q3map_material Glass
	qer_trans	0.5
	
	{{
		map $lightmap
		alphaGen const 0.15
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}}
	{{
		map textures/colors/crystal_env
		blendFunc GL_DST_COLOR GL_ONE
		rgbGen const ( {r:.6f} {g:.6f} {b:.6f} )
		tcGen environment
	}}
}}
""",
"""
textures/colors/gs_{c}
{{
	qer_editorimage textures/editor/gs_{c}
	surfaceparm	nonopaque
	surfaceparm	forcefield
	surfaceparm	trans
	q3map_material Glass
	qer_trans	0.5
	
	{{
		map $whiteimage
		alphaGen const 0.25
		rgbGen const ( {r:.6f} {g:.6f} {b:.6f} )
		blendFunc GL_DST_COLOR GL_ONE_MINUS_DST_COLOR
	}}
}}
"""
}

output = '// this file was automatically generated by generate_colors.py on {}\n\n'.format(datetime.datetime.now());

templates = [
	{
		'path': 'color_c_template.png',
		'name': 'c_{c}',
	},	{
		'path': 'color_ld_template.png',
		'name': 'ld_{c}',
	},	{
		'path': 'color_lvd_template.png',
		'name': 'lvd_{c}',
	},	{
		'path': 'color_fb_template.png',
		'name': 'fb_{c}',
	},	{
		'path': 'color_l_template.png',
		'name': 'l_{c}',
	},	{
		'path': 'color_m_template.png',
		'name': 'm_{c}',
	},	{
		'path': 'color_n_template.png',
		'name': 'n_{c}',
	},	{
		'path': 'color_lvb_template.png',
		'name': 'lvb_{c}',
	},	{
		'path': 'color_lb_template.png',
		'name': 'lb_{c}',
	},	{
		'path': 'color_cu_template.png',
		'name': 'cu_{c}',
	},	{
		'path': 'color_cg_template.png',
		'name': 'cg_{c}',
	},	{
		'path': 'color_cgl_template.png',
		'name': 'cgl_{c}',
	},	{
		'path': 'color_g_template.png',
		'name': 'g_{c}',
	},	{
		'path': 'color_s_template.png',
		'name': 's_{c}',
	},	{
		'path': 'color_gs_template.png',
		'name': 'gs_{c}',
	},
]

def lcol(c):
	return c + ((1 - c) / 2)

def dcol(c):
	return c / 2

def lighten(c, v):
	return c + ((1 - c) * v)
	
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
	for template in templates:
		templimg = Image.open('./templates/{}'.format(template['path']))
		data = templimg.load()
		for x in range(templimg.width):
			for y in range(templimg.height):
				if data[x, y][0]:
					data[x, y] = (int(r * 256), int(g * 256), int(b * 256))
				elif r+g+b <= 0.5:
					data[x, y] = (int(lighten(r, 0.25) * 256), int(lighten(g, 0.25) * 256), int(lighten(b, 0.25) * 256))
				else:
					data[x, y] = (int(r * 0.75 * 256), int(g * 0.75 * 256), int(b * 0.75 * 256))
		obj = {'c':color}
		templimg.save('./textures/editor/{}.png'.format(template['name'].format(**obj)))
	img = Image.new('RGB', (64, 64), 'rgb({},{},{})'.format(round(colors[color][0] * 255), round(colors[color][1] * 255), round(colors[color][2] * 255)))
	img.save('./textures/editor/lightimage_{}.png'.format(color))
	output += "\n// ==== {} ====\n".format(color.upper())
	for set in sets:
		obj = {
			'c':color, 
			'r':colors[color][0], 'g':colors[color][1], 'b':colors[color][2],
			'rl':lcol(colors[color][0]), 'gl':lcol(colors[color][1]), 'bl':lcol(colors[color][2]),
			'rd':dcol(colors[color][0]), 'gd':dcol(colors[color][1]), 'bd':dcol(colors[color][2]),
		}
		output += set.format(**obj)

print('writing shader file...')
f = open('./shaders/colors.shader', 'w')
f.write(output)
f.close()
print('done')
