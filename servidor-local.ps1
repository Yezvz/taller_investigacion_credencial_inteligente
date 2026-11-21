# Servidor HTTP local ligero para probar en celulares y PC (Sin dependencias externas)
param (
    [int]$Port = 8080
)

# Obtener IP local de Wi-Fi o red
$localIp = (Get-NetIPAddress -AddressFamily IPv4 -InterfaceAlias "*Wi-Fi*","*Ethernet*" -ErrorAction SilentlyContinue | Where-Object { $_.IPAddress -notmatch '^169\.' -and $_.IPAddress -notmatch '^127\.' } | Select-Object -First 1).IPAddress
if (-not $localIp) { $localIp = "127.0.0.1" }

$root = $PSScriptRoot
if (-not $root) { $root = Get-Location }

$listener = New-Object System.Net.HttpListener
$listener.Prefixes.Add("http://*:$Port/")
$listener.Prefixes.Add("http://localhost:$Port/")

try {
    $listener.Start()
} catch {
    # Si falla con '*' por permisos en Windows, escuchar en localhost y la IP directamente
    $listener = New-Object System.Net.HttpListener
    $listener.Prefixes.Add("http://localhost:$Port/")
    $listener.Prefixes.Add("http://$localIp`:$Port/")
    $listener.Start()
}

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "  CREDENCIAL INTELIGENTE ITTLA - SERVIDOR LOCAL ACTIVO" -ForegroundColor Green
Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "  En tu PC:     http://localhost:$Port" -ForegroundColor Yellow
Write-Host "  En tu CELULAR (misma red Wi-Fi):" -ForegroundColor White
Write-Host "                http://$localIp`:$Port" -ForegroundColor Green
Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "  Presiona Ctrl+C en esta ventana para detener el servidor." -ForegroundColor Gray
Write-Host ""

$mimeTypes = @{
    ".html" = "text/html; charset=utf-8"
    ".css"  = "text/css; charset=utf-8"
    ".js"   = "application/javascript; charset=utf-8"
    ".json" = "application/json; charset=utf-8"
    ".png"  = "image/png"
    ".jpg"  = "image/jpeg"
    ".jpeg" = "image/jpeg"
    ".svg"  = "image/svg+xml"
    ".ico"  = "image/x-icon"
}

while ($listener.IsListening) {
    try {
        $context = $listener.GetContext()
        $request = $context.Request
        $response = $context.Response

        $urlPath = $request.Url.LocalPath
        if ($urlPath -eq "/" -or $urlPath -eq "") {
            $urlPath = "/index.html"
        }

        $filePath = Join-Path $root ($urlPath.TrimStart("/").Replace("/", [System.IO.Path]::DirectorySeparatorChar))

        if (Test-Path $filePath -PathType Leaf) {
            $ext = [System.IO.Path]::GetExtension($filePath).ToLower()
            $contentType = if ($mimeTypes.ContainsKey($ext)) { $mimeTypes[$ext] } else { "application/octet-stream" }

            $bytes = [System.IO.File]::ReadAllBytes($filePath)
            $response.ContentType = $contentType
            $response.ContentLength64 = $bytes.Length
            $response.StatusCode = 200
            $response.OutputStream.Write($bytes, 0, $bytes.Length)
        } else {
            $response.StatusCode = 404
            $errBytes = [System.Text.Encoding]::UTF8.GetBytes("404 - Archivo no encontrado")
            $response.OutputStream.Write($errBytes, 0, $errBytes.Length)
        }

        $response.Close()
    } catch {
        # Ignorar desconexiones abruptas del cliente
    }
}
