# Core dumps analysis

## Set up `dump_scene` and `dump_path` parameters

There are two ways to configure the parameters that enable the functionality of generating dump files for exception handlers: using an `acl.json` file and the `acl.init` function or using environment variables.

### Using `acl.json` file and `acl.init` function

The file should be located in the same folder as your Python script.
File content should be the following:

```json
{
  "dump": {
    "dump_path": "./dump_output",
    "dump_scene": "aic_err_detail_dump"
  }
}
```

Path to⁣ `acl.json` is passed as an argument of the init function. For details, see the doc.
Python file `vector_add.py`

```python
import torch
import triton
import triton.language as tl
import acl

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

    # Wrong addr is the reason of device-side failure
    bad_ptr = output_ptr + 0x100000000
    tl.store(bad_ptr + offsets, output)

    # CORRECT store call should be the following
    #tl.store(output_ptr + offsets, output)


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
    acl_config_path = "./acl.json"
    print(f"Path to acl config: {acl_config_path}")
    ret = acl.init(acl_config_path)
    if ret != 0:
        raise RuntimeError(f"acl.init failed, ret={ret}")

    a = [1.0, 2.0, 3.0, 4.0]
    b = [2.0, 4.0, 6.0, 8.0]

    x = torch.tensor(a, device = 'npu')
    y = torch.tensor(b, device = 'npu')

    output_triton = vector_add(x, y)

    print("Output: ", output_triton)

    acl.finalize()
```
Path to⁣ `acl.json` is passed as an argument of the init function. For details, see [the doc](https://www.hiascend.com/document/detail/en/canncommercial/800/appdevg/aclpythondevg/aclpythondevg_0007.html).

### Using environment variables

```shell
export ASCEND_DUMP_SCENE="aic_err_detail_dump"
export ASCEND_DUMP_PATH="/workspace/dump_output"
```

## Check env

```shell
export TRITON_ALWAYS_COMPILE=1
```

## Running python script

The script should fail; a core dump file will be saved to the `./dump_output` directory.

```shell
python3 vector_add.py
```

![image.png](https://raw.gitcode.com/user-images/assets/9608702/fda21c4b-b241-41c5-91a7-5b60c21c65f6/image.png 'image.png')

## Running msDebug to investigate the core dump file

`TRITON_DUMP_DIR=/workspace/triton_dumps` ⁣points to the dir that contains the `add_kernel.npubin` binary kernel for the Ascend NPU (`TRITON_KERNEL_DUMP=1`).

![image.png](https://raw.gitcode.com/user-images/assets/9608702/c807e44b-1471-4dff-a0cf-1c33c8816653/image.png 'image.png')

Execute the following command:

```shell
msdebug --core /workspace/scripts/dump_output/extra-info/data-dump/0/add_kernel.46.0.20260519121714914.core
```

Your output should look like the following:

![image.png](https://raw.gitcode.com/user-images/assets/9608702/e06fa395-addc-4513-829f-12257f6898c6/image.png 'image.png')

## msDebug commands

### Ascend info summary

```shell
ascend info summary
```

![image.png](https://raw.gitcode.com/user-images/assets/9608702/b588ae84-504f-4ebb-aaea-4878eb8eb7f6/image.png 'image.png')

### Read memory

```shell
x -m GM -f int32_t[] 0x12c100400000 -s 72 -c 1
```

![image.png](https://raw.gitcode.com/user-images/assets/9608702/8c3d0c61-b2b0-4b34-8612-fad253bf340f/image.png 'image.png')

### Register read

```shell
register read -a
```

![image.png](https://raw.gitcode.com/user-images/assets/9608702/aaff2340-32a2-4f70-bd39-085dbdca6a4a/image.png 'image.png')

### Backtrace

```shell
bt
```

![image.png](https://raw.gitcode.com/user-images/assets/9608702/9f9a63fa-4af1-4fcb-8a6e-629bd1b80c44/image.png 'image.png')

### Select a frame

```shell
frame select 1
```

![image.png](https://raw.gitcode.com/user-images/assets/9608702/bbffd178-8e3d-4f01-8cea-304b80481984/image.png 'image.png')