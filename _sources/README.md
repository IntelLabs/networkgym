# Build Documentation Website

## Install Dependencies
- (Optional) Create a new virtual python environment.
```
python3 -m venv sphinx_venv
source sphinx_venv/bin/activate
```
- Install Required Libraries `pip install -r requirements.txt`

## Build Website
In the `docs` folder, run the following commands to export the static website in the `_build/html` folder.
```
make clean; make html
```

## Useful Tips
### Auto Generate rst Files
Run the following commands to generate rst files for `network_gym_client` library. The rst files contain modules, classes and functions.
```
rm network_gym_client*.rst
rm modules.rst
sphinx-apidoc -o . ../network_gym_client -d 2
```

### Add Classes and Functions to md Files
Add the classes and functions in the md (markdown) files. E.g.,

````
```{eval-rst}
.. autoclass:: network_gym_client.Env
```
````

