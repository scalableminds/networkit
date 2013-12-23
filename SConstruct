import os
import fnmatch
import ConfigParser

home_path = os.environ['HOME']

# SOURCE files (including executable) will be gathered here
srcDir = "src"
def getSourceFiles(target, optimize):
	source = []

	# walk source directory and find ONLY .cpp files
	for (dirpath, dirnames, filenames) in os.walk(srcDir):
	    for name in fnmatch.filter(filenames, "*.cpp"):
			source.append(os.path.join(dirpath, name))

	# exclude files depending on target, executables will be addes later
	if (target not in ["Tests"]):
		# exclude files matching following patterns
		xpatterns = ["*-X.cpp","*Unittests.cpp","*GTest.cpp","*Benchmark.cpp"]
		excluded = []
	else:
		# exclude files matching following patterns
		# only the target "Test" requires Benchmark and GTest files
		xpatterns = ["*-X.cpp","*Unittests.cpp"]
		excluded = []

	for pattern in xpatterns:
		for name in fnmatch.filter(source, pattern):
			excluded.append(name)

	print("excluded source files: {0}".format(excluded))
	source = [name for name in source if name not in excluded]

	# add executable
	if target == "CommunityDetection":
		source.append(os.path.join(srcDir, "CommunityDetection-X.cpp"))
	elif target == "DynCD":
		source.append(os.path.join(srcDir, "DynamicCommunityDetection-X.cpp"))
	elif target == "SelCD":
		raise Error("target SelCD currently disabled")  # cls
		# source.append(os.path.join(srcDir, "SelectiveCommunityDetection-X.cpp"))
	elif target == "Tests":
		source.append(os.path.join(srcDir, "Unittests.cpp"))
	#else case: error?	

	# create build directory for build configuration
	buildDir = ".build{0}".format(optimize)
	VariantDir(buildDir, srcDir, duplicate=0)

	# modify source paths for build directory
	source = [name.replace(srcDir + "/", buildDir + "/") for name in source]
	print(source)
	return source


# ENVIRONMENT

## read environment settings from configuration file

env = Environment()
confPath = "build.conf"
if not os.path.isfile(confPath):
	raise IOError("The configuration file `build.conf` does not exist. You need to create it.")

conf = ConfigParser.ConfigParser()
conf.read([confPath])     # read the configuration file

## compiler
cppComp = conf.get("compiler", "cpp", "gcc")
defines = conf.get("compiler", "defines", [])		# defines are optional
if defines is not []:
    defines = defines.split(",")

## includes
stdInclude = conf.get("includes", "std", "")      # includes for the standard library - may not be needed
gtestInclude = conf.get("includes", "gtest")
log4cxxInclude = conf.get("includes", "log4cxx")
if conf.has_option("includes", "tbb"):
	tbbInclude = conf.get("includes", "tbb", "")
else:
	tbbInclude = ""

## libraries
gtestLib = conf.get("libraries", "gtest")
log4cxxLib = conf.get("libraries", "log4cxx")
if conf.has_option("libraries", "tbb"):
	tbbLib = conf.get("libraries", "tbb", "")
else:
	tbbLib = ""

env["CC"] = cppComp
env["CXX"] = cppComp

env.Append(CPPDEFINES=defines)
env.Append(CPPPATH = [stdInclude, gtestInclude, tbbInclude]) #, log4cxxInclude
env.Append(LIBS = ["gtest"]) #, "log4cxx"
env.Append(LIBPATH = [gtestLib, tbbLib]) #, log4cxxLib
env.Append(LINKFLAGS = ["-std=c++11"])

## CONFIGURATIONS

commonCFlags = ["-c", "-fmessage-length=0", "-std=c99", "-fPIC"]
commonCppFlags = ["-std=c++11", "-Wall", "-c", "-fmessage-length=0", "-fPIC"]

debugCppFlags = ["-O0", "-g3"]
debugCFlags = ["-O0", "-g3"]

optimizedCppFlags = ["-O3", "-DNDEBUG"]
optimizedCFlags = ["-O3"]

profileCppFlags = ["-O2", "-DNDEBUG", "-g", "-pg"]
profileCFlags = ["-O2", "-DNDEBUG", "-g", "-pg"]


# select configuration
# custom command line options
AddOption("--optimize",
          dest="optimize",
          type="string",
          nargs=1,
          action="store",
          help="specify the optimization level to build (Debug, Release, Profile)")


try:
    optimize = GetOption("optimize")
except:
    print("ERROR: Missing option --optimize=<LEVEL>")
    exit()


# create build directory for build configuration
# modify source paths for build directory
# moved to getSourceFiles()

# append flags

#commmon flags
env.Append(CFLAGS = commonCFlags)
env.Append(CPPFLAGS = commonCppFlags)

#option to specify Logging type
AddOption("--logging",
          dest="logging",
          type="string",
          nargs=1,
          action="store",
          help="choose logging type: NOLOGGING or NOLOG4CXX")
try:
	logging = GetOption("logging")
	print("specified logging type is "+logging)
	if logging in ["NOLOGGING","NOLOG4CXX","SIMPLE"]:
		env.Append(CPPDEFINES=[logging])
except:
	print("No logging type specified, using LOG4CXX")
	env.Append(CPPPATH = [log4cxxInclude])
	env.Append(LIBS = ["log4cxx"])
	env.Append(LIBPATH = [log4cxxLib])



# openmp yes or no
AddOption("--openmp",
          dest="openmp",
          type="string",
          nargs=1,
          action="store",
          help="-fopenmp: yes or no")

openmp = GetOption("openmp")

if (openmp == "yes") or (openmp == None): # with OpenMP by default
    env.Append(CPPFLAGS = ["-fopenmp"])
    env.Append(LINKFLAGS = ["-fopenmp"])
elif (openmp == "no"):
    env.Append(LIBS = ["pthread"])
else:
    print("ERROR: unrecognized option --openmp=%s" % openmp)
    exit()

# optimize flags
if optimize == "D":
    env.Append(CFLAGS = debugCFlags)
    env.Append(CPPFLAGS = debugCppFlags)
elif optimize == "O":
    env.Append(CFLAGS = optimizedCFlags)
    env.Append(CPPFLAGS = optimizedCppFlags)
elif optimize == "P":
	 env.Append(CFLAGS = profileCFlags)
	 env.Append(CPPFLAGS = profileCppFlags)
else:
    print("ERROR: invalid optimize: %s" % optimize)
    exit()


# TARGET
AddOption("--target",
          dest="target",
          type="string",
          nargs=1,
          action="store",
          help="select target to build")


target = GetOption("target")
availableTargets = ["CommunityDetection","DynCD","SelCD","Core","Tests"]
if target in availableTargets:
	source = getSourceFiles(target,optimize)
	targetName = "NetworKit-{0}-{1}".format(target, optimize)
	if target == "Core":
		# do not append executable
		# env.Append(CPPDEFINES=["NOLOGGING"])
		env.Library("NetworKit-Core-{0}".format(optimize), source)
	else:
		env.Program(targetName, source)
else:
	print("ERROR: unknown target: {0}".format(target))
	exit()