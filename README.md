<div align="center">

# Dragon Age: Origins Ultrawide+
Work In Progress

</div>

DAO Ultrawide+ is an attempt at maintaining aspect ratio for displays above 16:9 to eliminate letterboxing
and potentially make improvements to the game's UI for use on ultrawide displays.\
Currently a proxy d3d9.dll is used to intercept the creation of the Direct3D device.

With current implementation there is severe image stretching and minor UI misalignment in dialogue scenes

![Dragon Age  Origins Screenshot 2024 11 12 - 17 18 25 28](https://github.com/user-attachments/assets/a07367ad-0d39-4042-b209-0f29c8eed71b)

For now this serves as a base for proxying the game's d3d9 usage, but it is actively being worked on.