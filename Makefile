##
#  Makefile FSM Project - Versão Final (Com Drivers LCD)
##

# -----------------------------------------------------------------------------
# 1. Configurações Básicas
# -----------------------------------------------------------------------------
PROGNAME=fsm_code

# SEU CAMINHO DO SDK
GECKOSDKDIR=/c/msys64/home/ageno/Gecko_SDK

# Dispositivo
PART=EFM32GG990F1024

# -----------------------------------------------------------------------------
# 2. Compilador e Flags
# -----------------------------------------------------------------------------
CC=arm-none-eabi-gcc
OBJCOPY=arm-none-eabi-objcopy
SIZE=arm-none-eabi-size

# --- DIRETÓRIOS CRÍTICOS DO SDK ---
# 1. Core e Device (Processador)
CMSISDIR=${GECKOSDKDIR}/platform/CMSIS/Core
CMSISDEVINCDIR=${GECKOSDKDIR}/platform/Device/SiliconLabs/EFM32GG/Include
EMLIBDIR=${GECKOSDKDIR}/platform/emlib/inc
COMMONDIR=${GECKOSDKDIR}/platform/common/inc

# 2. Drivers da Placa (LCD, etc)
KITDRIVERS=${GECKOSDKDIR}/hardware/kit/common/drivers
KITCONFIG=${GECKOSDKDIR}/hardware/kit/EFM32GG_STK3700/config

# Includes (Adiciona todas as pastas acima)
INCLUDEPATH= -I. \
             -I${CMSISDIR}/Include \
             -I${CMSISDEVINCDIR} \
             -I${EMLIBDIR} \
             -I${COMMONDIR} \
             -I${KITDRIVERS} \
             -I${KITCONFIG}

# Flags de Compilação
CFLAGS= -mcpu=cortex-m3 -mthumb -mfloat-abi=soft -std=c99 -Wall -g3 -O0
CFLAGS+= -D${PART} ${INCLUDEPATH}
CFLAGS+= --specs=nano.specs

# -----------------------------------------------------------------------------
# 3. Arquivos Fonte
# -----------------------------------------------------------------------------
# Compila todos os .c da pasta atual
SRCFILES=${wildcard *.c}
OBJDIR=gcc
OBJFILES=${addprefix ${OBJDIR}/,${SRCFILES:.c=.o}}

# Script de Linkagem
LINKERSCRIPT=${GECKOSDKDIR}/platform/Device/SiliconLabs/EFM32GG/Source/GCC/efm32gg.ld

# -----------------------------------------------------------------------------
# 4. Regras de Compilação
# -----------------------------------------------------------------------------
default: all

all: ${OBJDIR}/${PROGNAME}.bin size

${OBJDIR}/%.o: %.c
	@mkdir -p ${OBJDIR}
	@echo "Compilando $<"
	${CC} -c ${CFLAGS} -o $@ $<

${OBJDIR}/${PROGNAME}.axf: ${OBJFILES}
	@echo "Linkando $@"
	${CC} ${CFLAGS} -T ${LINKERSCRIPT} -Wl,--gc-sections -o $@ ${OBJFILES}

${OBJDIR}/${PROGNAME}.bin: ${OBJDIR}/${PROGNAME}.axf
	@echo "Gerando Binario $@"
	${OBJCOPY} -O binary $< $@

clean:
	rm -rf ${OBJDIR}

size: ${OBJDIR}/${PROGNAME}.axf
	${SIZE} $<

# Regra de Flash
flash: ${OBJDIR}/${PROGNAME}.bin
	echo "r" > flash.jlink
	echo "h" >> flash.jlink
	echo "loadbin ${OBJDIR}/${PROGNAME}.bin, 0" >> flash.jlink
	echo "r" >> flash.jlink
	echo "g" >> flash.jlink
	echo "exit" >> flash.jlink
	JLink -Device ${PART} -If SWD -Speed 4000 -CommanderScript flash.jlink