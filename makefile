WINDOWS_CURDIR := $(subst /,\,$(CURDIR))
WORKSPACEFOLDER:=${CURDIR}

SRCS:=$(wildcard ${WORKSPACEFOLDER}/src/*.cpp)

INCLUDES:=$(wildcard ${WORKSPACEFOLDER}/include/*.h)

INCLUDE_DIRS:=/I${WORKSPACEFOLDER}/include /IC:\Libraries\glfw-3.3.8.bin.WIN64\include /I${VULKAN_SDK}/Include
INCLUDE_DIRS+=/IC:\Libraries\tinyobjloader /IC:\Libraries\oneapi-tbb-2021.10.0\include

SHADER_DIR:=${WORKSPACEFOLDER}/shaders
COMPSHADER_PREFIX = ${SHADER_DIR}/spv/compshader
SHADERS:=${COMPSHADER_PREFIX}_deltaposition.spv ${COMPSHADER_PREFIX}_euler.spv ${COMPSHADER_PREFIX}_filtering.spv ${COMPSHADER_PREFIX}_lambda.spv
SHADERS+=${COMPSHADER_PREFIX}_positionupd.spv ${COMPSHADER_PREFIX}_postprocessing.spv ${COMPSHADER_PREFIX}_velocitycache.spv 
SHADERS+=${COMPSHADER_PREFIX}_velocityupd.spv ${COMPSHADER_PREFIX}_viscositycorr.spv ${COMPSHADER_PREFIX}_vorticitycorr.spv

SHADERS+=${SHADER_DIR}/spv/fragshader.spv ${SHADER_DIR}/spv/vertshader.spv

SHADERS+=${COMPSHADER_PREFIX}_calcellhash.spv ${COMPSHADER_PREFIX}_radixsort1.spv ${COMPSHADER_PREFIX}_radixsort2.spv ${COMPSHADER_PREFIX}_radixsort3.spv
SHADERS+=${COMPSHADER_PREFIX}_fixcellbuffer.spv ${COMPSHADER_PREFIX}_getngbrs.spv

LIB_PATHS:=/LIBPATH:${VULKAN_SDK}/Lib /LIBPATH:C:\Libraries\glfw-3.3.8.bin.WIN64\lib-static-ucrt /LIBPATH:C:\Libraries\oneapi-tbb-2021.10.0\lib\intel64\vc14 
LIBS:=/link ${LIB_PATHS} vulkan-1.lib glfw3dll.lib tbb.lib

BUILD_DIR:=${WORKSPACEFOLDER}\build

MACROS:=/D DEBUG

all:${BUILD_DIR}/main.exe ${SHADERS}

${BUILD_DIR}/main.exe:${SRCS} ${INCLUDES}
	@echo ${SRCS}
	@cl /std:c++17 /EHsc /Zi /Fo${BUILD_DIR}/ /Fe${BUILD_DIR}/main /Fd${BUILD_DIR}/main.pdb ${INCLUDE_DIRS} ${MACROS} ${SRCS} ${LIBS}
	
$(SHADER_DIR)/spv/vertshader.spv:$(SHADER_DIR)/glsl/shader.vert
	@${VULKAN_SDK}/Bin/glslc.exe $(SHADER_DIR)/glsl/shader.vert -o $(SHADER_DIR)/spv/vertshader.spv

$(SHADER_DIR)/spv/fragshader.spv:$(SHADER_DIR)/glsl/shader.frag
	@${VULKAN_SDK}/Bin/glslc.exe $(SHADER_DIR)/glsl/shader.frag -o $(SHADER_DIR)/spv/fragshader.spv

$(SHADER_DIR)/spv/compshader_lambda.spv:$(SHADER_DIR)/glsl/lambda.comp
	@${VULKAN_SDK}/Bin/glslc.exe $(SHADER_DIR)/glsl/lambda.comp -o $(SHADER_DIR)/spv/compshader_lambda.spv

$(SHADER_DIR)/spv/compshader_euler.spv:$(SHADER_DIR)/glsl/euler.comp
	@${VULKAN_SDK}/Bin/glslc.exe $(SHADER_DIR)/glsl/euler.comp -o $(SHADER_DIR)/spv/compshader_euler.spv

$(SHADER_DIR)/spv/compshader_deltaposition.spv:$(SHADER_DIR)/glsl/deltaposition.comp
	@${VULKAN_SDK}/Bin/glslc.exe $(SHADER_DIR)/glsl/deltaposition.comp -o $(SHADER_DIR)/spv/compshader_deltaposition.spv

$(SHADER_DIR)/spv/compshader_positionupd.spv:$(SHADER_DIR)/glsl/positionupd.comp
	@${VULKAN_SDK}/Bin/glslc.exe $(SHADER_DIR)/glsl/positionupd.comp -o $(SHADER_DIR)/spv/compshader_positionupd.spv

$(SHADER_DIR)/spv/compshader_velocitycache.spv:$(SHADER_DIR)/glsl/velocitycache.comp
	@${VULKAN_SDK}/Bin/glslc.exe $(SHADER_DIR)/glsl/velocitycache.comp -o $(SHADER_DIR)/spv/compshader_velocitycache.spv

$(SHADER_DIR)/spv/compshader_velocityupd.spv:$(SHADER_DIR)/glsl/velocityupd.comp
	@${VULKAN_SDK}/Bin/glslc.exe $(SHADER_DIR)/glsl/velocityupd.comp -o $(SHADER_DIR)/spv/compshader_velocityupd.spv

$(SHADER_DIR)/spv/compshader_vorticitycorr.spv:$(SHADER_DIR)/glsl/vorticitycorr.comp
	@${VULKAN_SDK}/Bin/glslc.exe $(SHADER_DIR)/glsl/vorticitycorr.comp -o $(SHADER_DIR)/spv/compshader_vorticitycorr.spv

$(SHADER_DIR)/spv/compshader_viscositycorr.spv:$(SHADER_DIR)/glsl/viscositycorr.comp
	@${VULKAN_SDK}/Bin/glslc.exe $(SHADER_DIR)/glsl/viscositycorr.comp -o $(SHADER_DIR)/spv/compshader_viscositycorr.spv

$(SHADER_DIR)/spv/compshader_filtering.spv:$(SHADER_DIR)/glsl/filtering.comp
	@${VULKAN_SDK}/Bin/glslc.exe $(SHADER_DIR)/glsl/filtering.comp -o $(SHADER_DIR)/spv/compshader_filtering.spv

$(SHADER_DIR)/spv/compshader_postprocessing.spv:$(SHADER_DIR)/glsl/postprocessing.comp
	@${VULKAN_SDK}/Bin/glslc.exe $(SHADER_DIR)/glsl/postprocessing.comp -o $(SHADER_DIR)/spv/compshader_postprocessing.spv

$(SHADER_DIR)/spv/compshader_calcellhash.spv:$(SHADER_DIR)/glsl/calcellhash.comp
	@${VULKAN_SDK}/Bin/glslc.exe $(SHADER_DIR)/glsl/calcellhash.comp -o $(SHADER_DIR)/spv/compshader_calcellhash.spv

$(SHADER_DIR)/spv/compshader_radixsort1.spv:$(SHADER_DIR)/glsl/radixsort1.comp
	@${VULKAN_SDK}/Bin/glslc.exe $(SHADER_DIR)/glsl/radixsort1.comp -o $(SHADER_DIR)/spv/compshader_radixsort1.spv

$(SHADER_DIR)/spv/compshader_radixsort2.spv:$(SHADER_DIR)/glsl/radixsort2.comp
	@${VULKAN_SDK}/Bin/glslc.exe $(SHADER_DIR)/glsl/radixsort2.comp -o $(SHADER_DIR)/spv/compshader_radixsort2.spv

$(SHADER_DIR)/spv/compshader_radixsort3.spv:$(SHADER_DIR)/glsl/radixsort3.comp
	@${VULKAN_SDK}/Bin/glslc.exe $(SHADER_DIR)/glsl/radixsort3.comp -o $(SHADER_DIR)/spv/compshader_radixsort3.spv

$(SHADER_DIR)/spv/compshader_fixcellbuffer.spv:$(SHADER_DIR)/glsl/fixcellbuffer.comp
	@${VULKAN_SDK}/Bin/glslc.exe $(SHADER_DIR)/glsl/fixcellbuffer.comp -o $(SHADER_DIR)/spv/compshader_fixcellbuffer.spv

$(SHADER_DIR)/spv/compshader_getngbrs.spv:$(SHADER_DIR)/glsl/getngbrs.comp
	@${VULKAN_SDK}/Bin/glslc.exe $(SHADER_DIR)/glsl/getngbrs.comp -o $(SHADER_DIR)/spv/compshader_getngbrs.spv

	

clean:
	@del /F /Q ${WINDOWS_CURDIR}\build\main.exe
	@del /F /Q ${WINDOWS_CURDIR}\shaders\spv\*.spv

