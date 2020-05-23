#ifdef INLProcDecl
#define INLProc(type, name, def, uname) type name = def;
#undef INLProcDecl
#endif

#ifdef INLProcReset
#define INLProc(type, name, def, uname) name.reset();
#undef INLProcReset
#endif

#ifdef INLProcPush
#define INLProc(type, name, def, uname) name.push();
#undef INLProcPush
#endif

#ifdef INLProcResolve
#define INLProc(type, name, def, uname) if (m_data->name.set_location(get_location(uname)) == -1) Com_Printf(S_COLOR_RED "programs::q3main: could not find uniform location for \"" #uname "\"\n");
#undef INLProcResolve
#endif

INLProc( uniform_mat4,  m_mvp,       qm::mat4_t::identity(), "mvp"         )
INLProc( uniform_float, m_time,      0,                      "time"        )
INLProc( uniform_mat3,  m_uv,        qm::mat3_t::identity(), "uvm"         )
INLProc( uniform_vec4,  m_color,     qm::vec4_t(1, 1, 1, 1), "q3color"     )
INLProc( uniform_bool,  m_turb,      false,                  "turb"        )
INLProc( uniform_vec4,  m_turb_data, qm::vec4_t(0, 0, 0, 0), "turb_data"   )
INLProc( uniform_uint,  m_lmmode,    0,                      "lm_mode"     )
INLProc( uniform_uint,  m_bones,     0,                      "ghoul2"      )
INLProc( uniform_uint,  m_mapgen,    0,                      "mapgen"      )
INLProc( uniform_vec3,  m_viewpos,   qm::vec3_t(0, 0, 0),    "view_origin" )
INLProc( uniform_uint,  m_tcgen,     0,                      "tcgen"       )
INLProc( uniform_mat4,  m_m,         qm::mat4_t::identity(), "m"           )
INLProc( uniform_uint,  m_genrgb,    0,                      "cgen"        )
INLProc( uniform_uint,  m_genalpha,  0,                      "agen"        )

#undef INLProc
