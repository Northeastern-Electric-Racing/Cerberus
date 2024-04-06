import importlib.util
import sys
from jinja2 import Environment, FileSystemLoader

# Full path to the directory containing the cangen module
EMBEDDED_BASE_PATH = "./Drivers/Embedded-Base"
module_name = "cangen"

# Full path to the cangen module file
module_path = f"{EMBEDDED_BASE_PATH}/{module_name}/__init__.py"

# Add the cangen directory to the system's path
sys.path.append(EMBEDDED_BASE_PATH)

# Load the module
spec = importlib.util.spec_from_file_location(module_name, module_path)
cangen = importlib.util.module_from_spec(spec)
spec.loader.exec_module(cangen)

# Load in Jinja Templates
environment = Environment(loader=FileSystemLoader("./"))
template = environment.get_template("Core/templates/dti.c")

#decode_data = open("./src/decode_data.rs", "w")
#master_mapping = open("./src/master_mapping.rs", "w")

mpu_messages = cangen.YAMLParser().parse(open(f"{EMBEDDED_BASE_PATH}/{module_name}/can-messages/bms.yaml", "r"))

content = template.render(
    can_msgs = mpu_messages.msgs,
    EMBEDDED_BASE_PATH = EMBEDDED_BASE_PATH
)

with open("Core/Src/dti.c", "w") as fh:
    fh.write(content)