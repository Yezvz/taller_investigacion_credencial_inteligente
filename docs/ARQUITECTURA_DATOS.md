# Estructura de datos — Credencial Inteligente ITTLA

## Alcance de esta versión

Esta versión organiza el almacenamiento local y el acceso de alumnos. **No incorpora una pantalla para dar de alta alumnos ni un perfil de administrador.** La función de alta queda disponible en el módulo de datos para una futura etapa, sin conectarla todavía a la interfaz.

## Almacenamiento

La PWA utiliza IndexedDB en el navegador del dispositivo. La base se llama `IttlaCredencialDB` y contiene tres almacenes:

- `alumnos`: datos académicos y de cuenta. `id_alumno` es la clave primaria y `matricula` tiene índice único.
- `credenciales`: UID NFC, estado y `id_alumno` relacionado.
- `asistencias`: movimientos de entrada/salida asociados a `id_alumno`, matrícula, UID y fecha.

La relación principal es `alumnos.id_alumno` → `credenciales.id_alumno` y `alumnos.id_alumno` → `asistencias.id_alumno`. El UID NFC identifica la tarjeta; `id_alumno` identifica al alumno dentro de los registros.

## Inicio de sesión

El acceso solicita matrícula y contraseña. La matrícula se busca en IndexedDB y la contraseña se verifica mediante PBKDF2 con SHA-256 y salt individual. No se guarda la contraseña en texto plano.

La opción **Recordar matrícula** guarda únicamente la matrícula en `localStorage`; nunca guarda la contraseña ni restaura una sesión. Cada apertura de la PWA requiere iniciar sesión.

Para el registro de demostración que ya existía en el proyecto:

- Matrícula: `22251022`
- Contraseña inicial: `alumno2026`

Si la base ya tenía el alumno de demostración sin contraseña almacenada, la inicialización asigna ese acceso inicial y guarda únicamente su hash. Si existía una contraseña antigua en texto plano, la migración la convierte a hash y elimina el campo anterior. Los hashes existentes no se reemplazan.

La autenticación local sirve para la demostración del proyecto, pero no equivale a autenticación centralizada de producción: los datos permanecen en el navegador y un usuario con control del dispositivo puede inspeccionar o alterar el almacenamiento local.

## Preparación para altas futuras

`js/database.js` contiene `registerStudent(studentData, rawUid)`. No está conectada a ningún formulario en esta versión. Cuando se decida implementar el alta, esa función:

1. exige matrícula y contraseña;
2. valida que la matrícula y el UID no estén duplicados;
3. genera un `id_alumno`;
4. guarda el hash de la contraseña;
5. registra alumno y credencial en una transacción de IndexedDB.

No se debe editar `index.html` para agregar cada alumno. La futura interfaz deberá invocar esa función y proporcionar los datos del nuevo registro.

## Datos de demostración

La inicialización de datos de ejemplo ocurre únicamente cuando el almacén `alumnos` está vacío. La migración no elimina alumnos, credenciales ni asistencias existentes. Los UID y asistencias de demostración que ya estaban definidos en el proyecto se conservan para no alterar la prueba funcional actual.

## BLE, NFC y firmware

En esta modificación no se editaron `js/bluetooth-service.js`, `js/attendance-service.js` ni el archivo Arduino. La lectura BLE/NFC y el registro de asistencias conservan sus módulos originales.

**Advertencia sobre el ZIP recibido:** el archivo `codigo-arduino/coidgoFuncional-credncial.ino` incluido en ese paquete declara el lector RC522. No se debe asumir que corresponde al firmware PN532 de otra versión del proyecto. Verificar el firmware y el módulo físico antes de cargarlo al ESP32.

## Actualización de PWA

El nombre de caché del Service Worker se incrementó a `ittla-credencial-pwa-v3` para que el navegador descarte los recursos estáticos anteriores y descargue los archivos actualizados.
