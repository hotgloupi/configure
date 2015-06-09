from __future__ import print_function
import sys
import subprocess
import os

@given('a system executable {exe}')
def step_impl(context, exe):
    binary = None
    if sys.platform.startswith('win'):
        try:
            binary = subprocess.check_output(["where", exe]).decode('utf8').strip()
        except:
            pass
    else:
        try:
            binary = subprocess.check_output(["which", exe]).decode('utf8').strip()
        except:
            pass

    if binary is None:
        print(
            "Skipping scenario", context.scenario,
            "(executable %s not found)" % exe,
            file = sys.stderr
        )
        context.scenario.skip("The executable '%s' is not present" % exe)
    else:
        print(
            "Found executable '%s' at '%s'" % (exe, binary),
            file = sys.stderr
        )

@then('{exe} is a static executable')
def step_impl(ctx, exe):
    if sys.platform.lower().startswith('darwin'):
        ctx.scenario.skip("Static runtime linking is not supported on OS X")
        return

    if sys.platform.startswith('win'):
        lines = subprocess.check_output(["dumpbin.exe", "/DEPENDENTS", exe]).decode('utf8').split('\r\n')
        for line in lines:
            if 'msvcrt' in line.lower():
                assert False, 'Found MSVCRT: %s' % line
    else:
        out = subprocess.check_output(["file", exe]).decode('utf8')
        assert 'statically linked' in out, "Not a static executable: %s" % out

