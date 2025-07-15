/****************************************************************************/
//    Copyright (C) 2023 Cosmos                                             //
//                                                                          //
//    This file is part of tnx3000                                             //
//                                                                          //
//    tnx3000 is free software: you can redistribute it and/or modify          //
//    it under the terms of the GNU General Public License as published by  //
//    the Free Software Foundation, either version 3 of the License         //
//                                                                          //
//    tnx3000 is distributed in the hope that it will be useful,               //
//    but WITHOUT ANY WARRANTY; without even the implied warranty of        //
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         //
//    GNU General Public License for more details.                          //
/****************************************************************************/

$input v_texcoord0

#include <bgfx/bgfx_shader.sh>
#include "tnx3000.common.sh"

SAMPLER2D(tex_0, 0);

void main()
{
	gl_FragColor = texture2D(tex_0, v_texcoord0.xy);
}
