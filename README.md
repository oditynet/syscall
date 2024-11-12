Builds:

```sh
 gcc syscall.c -static -nostdlib -fno-stack-protector -o syscall
```

 My version API of linux kernel can run functions:

   * `create clone`
   * `create clone3`
   * `print string`
   * `print numbers`
   * `sleep`
   * `exit`
