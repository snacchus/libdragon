#include <rsp_magma.inc>

MgBeginShaderUniforms
MgEndShaderUniforms

MgBeginShader
    # Bootstrap shader code and data
    # This code is only executed when the magma ucode has just been switched to.
    # Otherwise, if magma is already loaded, the shader code replaces this one.
    # In the latter case rspq will directly jump into the shader code when loading vertices.
    jal_and_j Mg_LoadShader, MgCmd_LoadVertices
MgEndShader
