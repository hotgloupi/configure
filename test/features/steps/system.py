from __future__ import print_function
import sys
import subprocess

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

