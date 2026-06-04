# Typical Case

## Breakpoints settings

**Overview**
This demonstrates how to set breakpoints and navigate between different lines of Python code.

**Preliminary preparations**

- You should have installed the Python =>3.11
- You should have installed the `torch` package => 2.8.0
- You should have installed [the Triton Ascend](https://gitcode.com/Ascend/triton-ascend) packages =>3.5.0 version
- To set the relevant environment variable configuration, please refer to the MindStudio Debugger Tool User Guide.
- Setting up the environment variable `TRITON_DISABLE_LINE_INFO` into `0` value

```bash
export TRITON_DISABLE_LINE_INFO=0
```

It is crucial if you want to set breakpoints using the format filename + line number

- Create a Python script named `vector_add.py`

```python
import torch
import triton
import triton.language as tl

@triton.jit
def add_kernel(
    x_ptr,
    y_ptr,
    output_ptr,
    n_elements,
    BLOCK_SIZE: tl.constexpr
):
    pid = tl.program_id(axis = 0)

    block_start = pid * BLOCK_SIZE
    offsets = block_start + tl.arange(0, BLOCK_SIZE)

    x = tl.load(x_ptr + offsets)
    y = tl.load(y_ptr + offsets)

    output = x + y

    tl.store(output_ptr + offsets, output)

def vector_add(x: torch.Tensor, y: torch.Tensor) -> torch.Tensor:

    n_elements = x.numel()

    output = torch.empty_like(x)

    BLOCK_SIZE = 128

    grid = lambda meta: (triton.cdiv(n_elements, meta['BLOCK_SIZE']), )

    add_kernel[grid](
        x, y, output,
        n_elements,
        BLOCK_SIZE=BLOCK_SIZE
    )

    return output

if __name__ == "__main__":
    a = [1.0, 2.0, 3.0, 4.0]
    b = [2.0, 4.0, 6.0, 8.0]

    x = torch.tensor(a, device = 'npu')
    y = torch.tensor(b, device = 'npu')

    output_triton = vector_add(x, y)

    print("Output", output_triton)
```

**Operating steps**

Start msdebug using the followng command

```bash
msdebug python3 vector_add.py
```

Run the script using command `run` or `r`

```bash
(msdebug) run
```

if the process finished successfully you will see the following line between others

```bash
Output tensor([ 3.,  6.,  9., 12.], device='npu:0')
```

Set a breakpoint 

```bash
(msdebug) b add_kernel$local
Breakpoint 1: where = device_debugdata_0`add_kernel + 84 at vadd.py:15:24, address = 0x0000124000000054
```

You can set multiple breakpoints

```bash
(msdebug) b vector_add.py:19
Breakpoint 2: where = device_debugdata_0`add_kernel + 188 at vector_add.py:19:16, address = 0x00001240000000bc
```

To see existed breakpoints use the `breakpoint list` command

```bash
(msdebug) breakpoint list
Current breakpoints:
1: name = 'add_kernel$local', locations = 1
  1.1: where = device_debugdata_0`add_kernel + 84 at vector_add.py:15:24, address = 0x0000124000000054, unresolved, hit count = 0

2: file = 'vector_add.py', line = 19, exact_match = 0, locations = 1
  2.1: where = device_debugdata_0`add_kernel + 188 at vector_add.py:19:16, address = 0x00001240000000bc, unresolved, hit count = 0
```

Or its verbose variant

```bash
(msdebug) breakpoint list -v
Current breakpoints:
1: name = 'add_kernel$local'
    1.1:
      module = device_debugdata_0
      compile unit = vector_add.py
      function = add_kernel
      location = /workspace/vector_add.py:15:24
      address = 0x0000124000000054
      resolved = false
      hardware = false
      hit count = 0


2: file = 'vector_add.py', line = 19, exact_match = 0
    2.1:
      module = device_debugdata_0
      compile unit = vector_add.py
      function = add_kernel
      location = /workspace/vector_add.py:19:16
      address = 0x00001240000000bc
      resolved = false
      hardware = false
      hit count = 0
```

Run process

```bash
(msdebug) r
Process 956 launched: '/usr/local/python3.11.14/bin/python3' (aarch64)
Process 956 stopped and restarted: thread 1 received signal: SIGCHLD
Process 956 stopped and restarted: thread 1 received signal: SIGCHLD
[Launch of Kernel add_kernel on Device 0]
1 location added to breakpoint 1
1 location added to breakpoint 2
Process 956 stopped
[Switching to focus on Kernel add_kernel, CoreId 5, Type aiv]
* thread #1, name = 'python3', stop reason = breakpoint 1.2
    frame #0: 0x0000124000000054 device_debugdata_11`add_kernel at vector_add.py:15:24
```

Here you can see that the process has stopped at the first breakpoint.

Go to the next breakpoint

```bash
(msdebug) c
Process 422 resuming
Process 422 stopped
[Switching to focus on Kernel add_kernel, CoreId 0, Type aiv]
* thread #1, name = 'python3', stop reason = breakpoint 2.2
    frame #0: 0x00001240000000bc device_debugdata_11`add_kernel at vector_add.py:19:16
```

To finish the process, you have to apply the `c` (continue) command again

```bash
(msdebug) c
Process 422 resuming
.........
Output tensor([ 3.,  6.,  9., 12.], device='npu:0')
Process 422 stopped and restarted: thread 1 received signal: SIGCHLD
Process 422 exited with status = 0 (0x00000000)
```

**Deleting a breakpoint**

```bash
(msdebug) breakpoint delete 2
1 breakpoints deleted; 0 breakpoint locations disabled.
(msdebug) breakpoint list
Current breakpoints:
1: name = 'add_kernel$local', locations = 2, resolved = 1, hit count = 1
  1.1: where = device_debugdata_0`add_kernel + 84 at vector_add.py:15:24, address = device_debugdata_0[0x0000000000000054], unresolved, hit count = 0
  1.2: where = device_debugdata_11`add_kernel + 84 at vector_add.py:15:24, address = 0x0000124000000054, resolved, hit count = 1
```

Now you can see that there is only one breakpoint.

**Navigation**

- n (next) -> thread step-over - a step without going inside the functions
- s (step)  ->  thread step-in - the step of going inside the functions
- finish -> thread step-out - exit the current function

```bash
Process 540 stopped
[Switching to focus on Kernel add_kernel, CoreId 23, Type aiv]
* thread #1, name = 'python3', stop reason = breakpoint 1.3
    frame #0: 0x0000124000000054 device_debugdata_22`add_kernel at vector_add.py:15:24
(msdebug) n
Process 540 stopped
[Switching to focus on Kernel add_kernel, CoreId 23, Type aiv]
* thread #1, name = 'python3', stop reason = step over
    frame #0: 0x0000124000000058 device_debugdata_22`add_kernel at vector_add.py:6
(msdebug) s
Process 540 stopped
[Switching to focus on Kernel add_kernel, CoreId 23, Type aiv]
* thread #1, name = 'python3', stop reason = step in
    frame #0: 0x000012400000005c device_debugdata_22`add_kernel at vector_add.py:15:24
```
