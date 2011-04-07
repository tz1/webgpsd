import SCons.Script

env = Environment()
env.Replace(CFLAGS = '-g -Wall')
#env.Replace(CFLAGS = '-O6 -I. -D_GNU_SOURCE -Wall')

env.webh = SCons.Script.Builder( action = "./html2head $TARGET", suffix = '.h', src_suffix = '.html', single_source=True )
#env['BUILDERS']['Htmlhead'] = env.webh
env.Append(BUILDERS = { 'Htmlhead' : env.webh })

#env['BUILDERS']['h'],add_src_builder(env.webh)

for t in ['radfmt','satstat','dogmap']:
    env.Htmlhead( t )

Program( 'webgpsd' , Split('gpsdata.c webgpsd.c web.c kmlzipper.c'), CPPPATH='.'  )
#CFLAGS+= -U_FORTIFY_SOURCE # -Wno-attributes

