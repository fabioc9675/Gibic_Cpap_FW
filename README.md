# Gibic_CPAP_FW
This project contains the structure to handle the firmware development for GIBIC CPAP device

Para visualizar los registros del los perifiricos durante el debug, es necesario
agregar la siguiente linea al archivo ${workspaceFolder}/.vscode/settings.json. La linea
debe quedar entre los signos {} si es justo antes de la llave de cierre, debe ir sin la ,

"idf.svdFilePath": "${workspaceFolder}/esp32s3.svd",

La version de empleada del IDF es la 5.3.0
