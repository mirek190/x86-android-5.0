#!/usr/bin/env python

# this script checks that patch comments comply with the process
# constraints of the tool:
# start from within a repo
# the tool will only check patches in the checked out branch
# more checks will appear soon

from xml.dom import minidom
from xml.dom.minidom import parseString
from optparse import OptionParser
from subprocess import *
import sys,os,re,errno

from xml.parsers.expat import ExpatError # for handling 401 from curl

# _FindRepo() is from repo script
# used to find the root a the repo we are currently in
GIT = 'git'                     # our git command
MIN_GIT_VERSION = (1, 5, 4)     # minimum supported git version
repodir = '.repo'               # name of repo's private directory
S_repo = 'repo'                 # special repo reposiory
S_manifests = 'manifests'       # special manifest repository
REPO_MAIN = S_repo + '/main.py' # main script
abstract = ""
BZ_product = ""

def _FindRepo():
  """Look for a repo installation, starting at the current directory.
  """
  dir = os.getcwd()
  repo = None

  while dir != '/' and not repo:
    repo = os.path.join(dir, repodir, REPO_MAIN)
    if not os.path.isfile(repo):
      repo = None
      dir = os.path.dirname(dir)
  return (repo, os.path.join(dir, repodir))

# split_patch splits a patch in 2 parts: comment and file_list
# in: file
# out: (string,string)
def split_patch(patchfile):
	try:
		patch = open(patchfile, 'r')
	except IOError:
		print "file", patch,"does not exist"
		sys.exit(errno.ENOENT)

	comment=[]
	file_list=[]
	start=True
	in_comment=False
	in_file_list=False
	for line in patch.readlines():
		if start==True and line.startswith("Subject:"):
			start=False
			in_comment=True
			line=line[len("Subject: "):]
		if in_comment and line.startswith("---"):
			in_comment=False
			in_file_list=True
			continue
		if in_comment:
			comment.append(line)
			continue
		if in_file_list and not line.strip() == "" and line[1].isdigit():
			in_file_list=False
			break
		if in_file_list and not line.strip() == "":
			fields=line[1:].split(" ")
			file_list.append(fields[0])

	return (comment,file_list)

# split_comment splits a text in paragraph (separation = blank line)
# in: string
# out: list of lines
def split_comment(comment):
	res=[]
	parag=[]
	for l in comment:
		if l.strip()!="":
			parag.append(l)
		else:
			res.append(parag)
			parag=[]
	if l.strip()!="":
		res.append(parag)
	return res

# get_change_id extracts the change ID from a list of lines
# in: list of lines
# out: string
def get_change_id(lines):
	change_id=""
	for l in lines:
		if str(l).startswith("Change-Id: "):
			change_id=l[len("Change-Id: "):]
	return change_id

# get_info_bug retrieves infos from the BZ server
# in: bug number
# out: 0 if the BZ exists
#      -1 on error
def get_info_bug(bug, bypassbzstatus = False):
  global abstract
  #print "DEBUG bypassbzstatus %s" % (bypassbzstatus)
  global BZ_product
  result = 0
  try:
    p = Popen("curl \"http://umgbugzilla.sh.intel.com:41006/show_bug.cgi?id=%s&ctype=xml\" --netrc --silent" %(bug),
      shell=True, bufsize=10000, stdin=PIPE, stdout=PIPE, stderr=PIPE, close_fds=True)
    (child_stdin, child_stdout, child_stderr) = (p.stdin, p.stdout, p.stderr)
    tmp = ""
    for i in child_stdout.readlines():
      tmp += i
    # if curl catches 401 Authorization required the next line will raise
    # exception (ExpatError) which is handled
    dom3 = parseString(tmp)
    reflist = dom3.getElementsByTagName('bug')
    error = reflist[0].hasAttribute('error')
    if error == True:
        #print "BZ:", bug, "doesn't exist"
        abstract += "BZ: %s doesn't exist\n" % (bug)
        return 1
    reflist = dom3.getElementsByTagName('bug_id')
    bug_id = reflist[0].childNodes[0].nodeValue
    #print "BZ: ", bug_id
    reflist = dom3.getElementsByTagName('bug_status')
    status = reflist[0].childNodes[0].nodeValue
    #print "    status:", status
    reflist = dom3.getElementsByTagName('product')
    product = reflist[0].childNodes[0].nodeValue
    #print "    product:", product
    reflist = dom3.getElementsByTagName('version')
    version = reflist[0].childNodes[0].nodeValue
    #print "    version:", version
    flags = dom3.getElementsByTagName('flag')
    if options.product and product != BZ_product:
        #print "    BZ_product:", BZ_product
        abstract += "BZ: %s (%s) not in the right product; it should be in %s product\n" % (bug, product, BZ_product)
        result = 1
    if not bypassbzstatus and options.version and version != BZ_version:
        #print "    BZ_version:", BZ_version
        abstract += "BZ: %s (%s) not in the right version; it should be in %s version\n" % (bug, version, BZ_version)
        result = 1
    if not bypassbzstatus and options.states_list and not status in bz_state_list:
        #print "BZ:", bug, "not in the right status; it should be in this list:", bz_state_list
        abstract += "BZ: %s (%s) not in the right status; it should be in this list: %s\n" % (bug, status, str(bz_state_list))
        result = 1

  except ExpatError:
    abstract += "Cannot authorize connection to bugzilla. Please check netrc.\n"
    return 1
  except:
    #print "BZ:", bug, "doesn't exist"
    abstract += "BZ: %s doesn't exist\n" % (bug)
    import traceback
    traceback.print_exc()
    return 1
  return result

# check_comment verifies if a comment complies with process rules (it can be updated/enriched over time)
# in: string
# out: (string,string,string) (report status,subject,change_id)
# current checks:
#	- 2nd line is blank
#	- 3rd line (start of comment body) starts with: BZ nnnn
#	- last paragraph contains change_id
def check_comment(comment):
    global abstract
    bypassbzstatus=False
    paragraphs=split_comment(comment)
    subject=paragraphs[0][0].strip()
    #print "subject %s" % (subject)
    backportpattern=r'\[PATCH.*\] \[PORT FROM .*\].*'
    n=re.match(backportpattern,subject)
    if n:
      bypassbzstatus=True
    m = None
    check_ok=True
    report=""
    change_id=""
    try:
        bzline=paragraphs[1][0].strip()
        bzpattern=r'BZ:?\s*(?P<num>.*)'
        m=re.match(bzpattern,bzline)
        change_id=get_change_id(paragraphs[-1])
    except:
        pass
    if m:
        bzlist=re.split('\s+',m.group("num"))
#        print "DEBUG bzlist=",bzlist
        if ask_bz_server:
            for i in bzlist:
                if get_info_bug(i,bypassbzstatus):
                    report="\n".join([report,"    Error: Bugzilla id (%s) is not correct" %(i)])
    else:
        report="\n".join([report,"    Error: first line of comment body is not a Bugzilla ID (BZ: nnnn)"])
        abstract += "Error: first line of comment body is not a Bugzilla ID (BZ: nnnn) for this patch: %s\n" % (change_id)
    if change_id=="":
        report="\n".join([report,"    Error: last paragraph of comment body should have gerrit Change-ID"])
        abstract += "Error: last paragraph of comment body should have gerrit Change-ID: Find yourself this patch (as there is no Change-Id !)\n"
    if report=="":
        report="    Comment OK"
    else:
        report=report[1:]
    return (report,subject,change_id)

# main

# parse options
parser = OptionParser()

parser.add_option("-f", "--force",
                  action="store_true", dest="force", default=False,
                  help="don't ask before deleting patches at root of repo")

parser.add_option("-b", "--bugzilla",
                  action="store_true", dest="bugzilla", default=False,
                  help="ask Bugzilla server for status on each bug id")

parser.add_option("-s", "--states-list",
                  action="store", dest="states_list", default=None,
                  help="specify the authorized states for the BZ status,"
                       "         ex: -s 'IMPLEMENTED RESOLVED'")

parser.add_option("-p", "--product",
                  action="store", dest="product", default=None,
                  help="specify the wanted value for the BZ product field")

parser.add_option("-v", "--version",
                  action="store", dest="version", default=None,
                  help="specify the wanted value for the BZ version field")

(options, args) = parser.parse_args()
ask_before_delete=not options.force
ask_bz_server = options.bugzilla
if options.states_list:
    ask_bz_server=True
    bz_state_list=options.states_list.split()

if options.product:
    ask_bz_server=True
    BZ_product = options.product
    #print "DEBUG: BZ_product=%s" % (BZ_product)

if options.version:
    ask_bz_server=True
    BZ_version = options.version
    #print "DEBUG: BZ_version=%s" % (BZ_version)

# get the reference manifest (.repo/manifest.xml)
repo=_FindRepo()[1]
manifest=repo+'/manifest.xml'

if (not os.path.exists(manifest)):
	print "Error: no manifest in repo",repo
	sys.exit(errno.ENOENT)

# cd to repo root (repo format-patch writes patch files at the repo root)
os.chdir(os.path.dirname(repo))

# ask to remove existing patches
files=os.listdir(".")
old_patches=[ patch.strip() for patch in files if re.match("\d{4}\-.*\.patch", patch.strip()) ]
if len(old_patches) != 0:
	if ask_before_delete:
		print "There are old patches in %s:" %(os.getcwd())
		for old_patch in old_patches:
			print "    ",old_patch
		answer = raw_input("Are you OK to remove them? (y/n)")
		if not (answer == "y" or answer == "Y"):
			print "Error: please move your old patches before running this script"
			sys.exit(errno.EPERM)
	for old_patch in old_patches:
		try:
			ret=os.remove(old_patch)
		except OSError:
			print "Error: impossible to delete %s" %(old_patch)
			sys.exit(errno.EPERM)

# list patches to be checked
p5 = Popen(["repo", "format-patch", manifest], stdout=PIPE)
output=p5.communicate()[0]
print output
patches = []
for l in output.split('\n'):
    l = l.strip()
    if l.endswith(".patch") and os.path.isfile(l):
        patches.append(l)
if len(patches) == 0:
	print "Warning: no patch to check (Maybe already merged)"
	sys.exit(0)
global_status=0

private_file=False
private_files="private_files.log"
if os.path.exists(private_files):
    os.remove(private_files)
# check patches
print patches
print "############## REPORT START #################"
for patch in patches:
    try:
        res = os.system("vendor/intel/release/check-modified-makefiles.sh %s %s" % (patch, private_files))
        if res:
            private_file=True
    except:
        print "exception catched in check-modified-makefiles.sh\n"
    try:
        (comment,file_list)=split_patch(patch)
    except:
        continue
    (report,subject,change_id)=check_comment(comment)
    if report != "    Comment OK":
        global_status=errno.EAGAIN
    print "checking patch %s ..." %(subject)
    print >>sys.stderr, report
    print
    print"Change-Id: %s" %(change_id.strip())
    print
    print"File list:"
    for f in file_list:
        print f
    print"------------------------------------"
    print

if private_file:
    #global_status=errno.EAGAIN
    p = open(private_files)
    for line in p.readlines():
        pattern=r'WARNING:(.*)'
        m = re.match(pattern, line)
        if m:
            #abstract += "ERROR: %s \n" % m.group(1)
            abstract += "WARNING: %s \n" % m.group(1)

print "############## REPORT END ###################"
print
print >>sys.stderr, abstract

sys.exit(global_status)
