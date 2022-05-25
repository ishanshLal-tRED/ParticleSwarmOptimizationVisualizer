Param(
    [Parameter(Mandatory,
    HelpMessage="Enter one or more shader files to compile with spir-v (Output file will be [filename].sprv)")]
    [string[]]
    $Files
)

$VK_SDK = $env:VULKAN_SDK
$GLSLC = "$VK_SDK/Bin/glslc.exe"
$Files | ForEach{
    $object = (Get-Item -Path "$_")
    $file_name = $object.BaseName
    $full_path = $object.FullName
    $dir_path  = $object.Directory
    Invoke-Expression "$GLSLC `"$full_path`" -o `"$dir_path/$file_name.sprv`""
}

# TODO: add functionality to compile every shader inside a folder