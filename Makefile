
# VERBOSE can be set to 1 to echo all commands performed by this makefile
ifeq ($(VERBOSE),1)
	VB=''
else
	VB=@
endif

DEV_PYTHON_VIRTUAL_ENV = .dev_venv

.PHONY: dev-init generate-dev-requirements run-precommit-on-all-files run-precommit-on-staged-files


##########################################################
# Targets for managing the local Python dev environment
##########################################################

dev-init:
	@echo [\(Re\)Creating Python virtual environment at '${DEV_PYTHON_VIRTUAL_ENV}/' for development needs]
	${VB}rm -fr ${DEV_PYTHON_VIRTUAL_ENV}
	${VB}python3 -m venv ${DEV_PYTHON_VIRTUAL_ENV}
	@echo [Initialize dev Python virtual environment with all needed dependencies]
	${VB}${DEV_PYTHON_VIRTUAL_ENV}/bin/python -m pip install -r dev_requirements.txt
	@echo [Installing pre-commit hooks]
	${VB}${DEV_PYTHON_VIRTUAL_ENV}/bin/pre-commit install

generate-dev-requirements:
	@echo "# FYI: This file was created by calling 'make $@'" > dev_requirements.txt
	${VB}${DEV_PYTHON_VIRTUAL_ENV}/bin/python -m pip freeze >> dev_requirements.txt


##########################################################
# Targets for manually running pre-commit tests
##########################################################

run-precommit-on-all-files:
	${VB}${DEV_PYTHON_VIRTUAL_ENV}/bin/pre-commit run --all-files

run-precommit-on-staged-files:
	${VB}${DEV_PYTHON_VIRTUAL_ENV}/bin/pre-commit run
