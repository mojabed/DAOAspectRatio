<div align="center">

# Dragon Age: Origins Ultrawide+
Work In Progress

</div>

DAO Ultrawide+ is an attempt at maintaining aspect ratio for displays above 16:9 to eleminate letterboxing,
and potentially make improvements to the game's UI for use on ultrawide displays.\
Currently a proxy d3d9.dll is used to intercept the creation of the Direct3D device.

The SetViewport method is hooked to adjust the desired aspect ratio, but this causes image stretching
and bugs with the UI.

For now this serves as a base for proxying the game's d3d9 usage, but it is actively being worked on.

The code is ugly right now, but I wanted something to get me going. I will implement proper OOP standards
in the future.