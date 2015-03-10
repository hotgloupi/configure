import os
import shlex
import sys
import time

@then('it should pass')
def step_impl(ctx):
    pass

@given('a temporary directory')
def step_impl(ctx):
    assert os.path.isdir(ctx.directory)

@then('configure failed')
def step_impl(ctx):
    assert not ctx.initialized

@when('the build is configured')
@then('the build is configured')
def impl(ctx):
    assert ctx.configured

@when('I configure the build')
def step_impl(ctx):
    ctx.configured = ctx.cmd(ctx.configure_exe, 'build') == 0

@when('I configure')
@then('I configure')
def step_impl(ctx):
    ctx.configured = ctx.cmd('configure') == 0

@then('A .config directory is created')
def step_impl(ctx):
    assert ctx.initialized
    assert os.path.isdir('.config')

@then('a project config file is created')
def step_impl(ctx):
    assert ctx.initialized
    assert os.path.isfile('configure.lua')

@given('a project configuration')
def step_impl(ctx):
    ctx.execute_steps(u"Given a temporary directory")
    with open("configure.lua", 'w') as f:
        f.write(ctx.text)
    ctx.initialized = True


@given('a source file {filename}')
@when('a source file {filename}')
def step_impl(ctx, filename):
    if sys.platform.startswith('darwin') and os.path.exists(filename):
        time.sleep(1)
    with open(filename, 'w') as f:
        if sys.platform.startswith('win'):
            ctx.text = ctx.text.replace('\r','')
        f.write(ctx.text)
    with open(filename, 'rU') as f:
        print("Source file", filename, "content:")
        print(f.read())

@when('I build everything')
def step_impl(ctx):
    assert ctx.configured
    ctx.built = ctx.cmd(ctx.configure_exe, 'build', '--build') == 0

@then('I can launch {exe}')
def step_impl(ctx, exe):
    assert ctx.built
    assert ctx.cmd(os.path.join(ctx.directory, 'build', exe)) == 0

@when('I configure and build')
def step_impl(ctx):
    ctx.execute_steps(
        u"""
        When I configure the build
        And I build everything
        """
    )

@when('I try configure with {args}')
def impl(ctx, args):
    ctx.configured = ctx.cmd(ctx.configure_exe, *tuple(shlex.split(args))) == 0

@when('I configure with {args}')
def impl(ctx, args):
    ctx.execute_steps(u"When I try configure with %s" % args)
    assert ctx.configured
