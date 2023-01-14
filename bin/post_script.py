Import("env", "projenv")

if 'stm32' in env['PIOPLATFORM'] and "boot" not in env["PIOENV"]:

    size = env.BoardConfig().get('upload.maximum_size') - 0x1000

    for e in [ env, projenv ]:
        e.Replace(LINKFLAGS = [i for i in e['LINKFLAGS']
            if 'LD_MAX_SIZE' not in i and 'LD_FLASH_OFFSET' not in i])
        e.Append(LINKFLAGS =
                 ["-Wl,--defsym=LD_FLASH_OFFSET=0x1000",
                  f"-Wl,--defsym=LD_MAX_SIZE={size}" ])
