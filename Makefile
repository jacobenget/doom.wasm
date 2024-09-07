
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

dev-init: | ${DEV_PYTHON_VIRTUAL_ENV}
	@echo [Installing pre-commit hooks]
	${VB}( \
		source ${DEV_PYTHON_VIRTUAL_ENV}/bin/activate; \
		pre-commit install; \
	)

${DEV_PYTHON_VIRTUAL_ENV}:
	@echo [Creating Python virtual environment at '${DEV_PYTHON_VIRTUAL_ENV}/' for development needs]
	${VB}python3 -m venv ${DEV_PYTHON_VIRTUAL_ENV}
	${VB}( \
		source ${DEV_PYTHON_VIRTUAL_ENV}/bin/activate; \
		echo [Initialize dev Python virtual environment with all needed dependencies]; \
		python -m pip install -r dev_requirements.txt; \
	)

# Any changes made to the local Python dev environment must be reflected in dev_requirements.txt.
# Updating dev_requirements.txt to match the package versions being used in the Python dev environment is done via this target:
generate-dev-requirements: | ${DEV_PYTHON_VIRTUAL_ENV}
	${VB}> dev_requirements.txt
	@echo "##################################################################################################" >> dev_requirements.txt
	@echo "#" >> dev_requirements.txt
	@echo "# This file is used to manage which Python modules are installed in the virtual environment that lives at ${DEV_PYTHON_VIRTUAL_ENV}/" >> dev_requirements.txt
	@echo "#" >> dev_requirements.txt
	@echo "# WARNING: This file was auto-generated, likely by calling something similar to 'make $@'." >> dev_requirements.txt
	@echo "# Any manual edits made to this file will probably be clobbered!" >> dev_requirements.txt
	@echo "#" >> dev_requirements.txt
	@echo "# If you wish to change which Python modules are installed in the virtual environment at ${DEV_PYTHON_VIRTUAL_ENV}/" >> dev_requirements.txt
	@echo "# you should, instead, just manually install/uninstall modules in that virtual environment as you see fit" >> dev_requirements.txt
	@echo "# (see here if you're unsure of how to interact with Python virtual environment: https://docs.python.org/3/tutorial/venv.html)" >> dev_requirements.txt
	@echo "# and then manually run 'make $@'." >> dev_requirements.txt
	@echo "#" >> dev_requirements.txt
	@echo "##################################################################################################" >> dev_requirements.txt
	@echo "" >> dev_requirements.txt
	${VB}( \
		source ${DEV_PYTHON_VIRTUAL_ENV}/bin/activate; \
		python -m pip freeze >> dev_requirements.txt; \
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
