import subprocess

@then('build variable {key} in {build_dir} equals "{value}"')
def impl(ctx, key, value, build_dir):
    assert ctx.configured
    found_value = subprocess.check_output(
        [ctx.configure_exe, build_dir, '-P', key]
    ).decode('utf8').strip()
    assert found_value == value, "Expected {key} == {value}, got {found_value}".format(**locals())
