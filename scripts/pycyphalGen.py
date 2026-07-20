import pycyphal

# The generated Python package will be placed into the 'my_generated_folder' directory
pycyphal.dsdl.compile(
    '../tool/cyphalMessages',
    output_directory='../generated'
)