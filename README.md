Builds:

```sh
 gcc syscall.c -static -nostdlib -fno-stack-protector -o syscall
```

 My version API of linux kernel have functions:

   * `create clone`
   * `create clone3`
   * `print string`
   * `print numbers`
   * `sleep`
   * `exit`

About sys call you can read on https://www.chromium.org/chromium-os/developer-library/reference/linux-constants/syscalls/
