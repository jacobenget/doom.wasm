
# VERBOSE can be set to 1 to echo all commands performed by this makefile
ifeq ($(VERBOSE),1)
	VB=
else
	VB=@
endif


##########################################################
# Targets for managing the local Python dev environment
##########################################################

DEV_PYTHON_VIRTUAL_ENV = .dev_virtualenv
# Dependencies in Python dev environment are managed via a Requirements file, stored as dev_requirements.txt
DEV_REQUIREMENTS_TXT = dev_requirements.txt

dev-init: | ${DEV_PYTHON_VIRTUAL_ENV}
	@echo [Installing pre-commit hooks]
	${VB}( \
		source ${DEV_PYTHON_VIRTUAL_ENV}/bin/activate; \
		pre-commit install; \
	)

dev-clean:
	${VB}( \
		source ${DEV_PYTHON_VIRTUAL_ENV}/bin/activate; \
		pre-commit uninstall; \
	)
	${VB} rm -fr ${DEV_PYTHON_VIRTUAL_ENV}

${DEV_PYTHON_VIRTUAL_ENV}:
	@echo [Creating Python virtual environment at '${DEV_PYTHON_VIRTUAL_ENV}/' for development needs]
	${VB}python3 -m venv ${DEV_PYTHON_VIRTUAL_ENV}
	${VB}( \
		source ${DEV_PYTHON_VIRTUAL_ENV}/bin/activate; \
		echo [Initialize dev Python virtual environment with all needed dependencies]; \
		python -m pip install -r ${DEV_REQUIREMENTS_TXT}; \
	)

# Generate/update dev requirements file to match the package versions being used in the Python dev environment
# (this is done via a .PHONY target because it's not clear how we could direct 'make' to determine when this
# file is truly out of date, so instead we just always assume this file is out of date)
generate-dev-requirements: | ${DEV_PYTHON_VIRTUAL_ENV}
	${VB}> ${DEV_REQUIREMENTS_TXT}
	@echo "##################################################################################################" >> ${DEV_REQUIREMENTS_TXT}
	@echo "#" >> ${DEV_REQUIREMENTS_TXT}
	@echo "# This file is used to manage which Python modules are installed in the virtual environment that lives at ${DEV_PYTHON_VIRTUAL_ENV}/" >> ${DEV_REQUIREMENTS_TXT}
	@echo "#" >> ${DEV_REQUIREMENTS_TXT}
	@echo "# WARNING: This file was auto-generated, likely by calling something similar to 'make $@'." >> ${DEV_REQUIREMENTS_TXT}
	@echo "# Any manual edits made to this file will probably be clobbered!" >> ${DEV_REQUIREMENTS_TXT}
	@echo "#" >> ${DEV_REQUIREMENTS_TXT}
	@echo "# If you wish to change which Python modules are installed in the virtual environment at ${DEV_PYTHON_VIRTUAL_ENV}/" >> ${DEV_REQUIREMENTS_TXT}
	@echo "# you should, instead, just manually install/uninstall modules in that virtual environment as you see fit" >> ${DEV_REQUIREMENTS_TXT}
	@echo "# (see here if you're unsure of how to interact with Python virtual environment: https://docs.python.org/3/tutorial/venv.html)" >> ${DEV_REQUIREMENTS_TXT}
	@echo "# and then manually run 'make $@'." >> ${DEV_REQUIREMENTS_TXT}
	@echo "#" >> ${DEV_REQUIREMENTS_TXT}
	@echo "##################################################################################################" >> ${DEV_REQUIREMENTS_TXT}
	@echo "" >> ${DEV_REQUIREMENTS_TXT}
	${VB}( \
		source ${DEV_PYTHON_VIRTUAL_ENV}/bin/activate; \
		python -m pip freeze >> ${DEV_REQUIREMENTS_TXT}; \
	)


##########################################################
# Targets for manually running pre-commit tests
##########################################################

run-precommit-on-all-files: | ${DEV_PYTHON_VIRTUAL_ENV}
	${VB}( \
		source ${DEV_PYTHON_VIRTUAL_ENV}/bin/activate; \
		pre-commit run --all-files; \
	)

run-precommit-on-staged-files: | ${DEV_PYTHON_VIRTUAL_ENV}
	${VB}( \
		source ${DEV_PYTHON_VIRTUAL_ENV}/bin/activate; \
		pre-commit run; \
	)


.PHONY: generate-dev-requirements run-precommit-on-all-files run-precommit-on-staged-files
