#import pycyphal
import yakut

# The generated Python package will be placed into the 'generated' directory
'''
pycyphal.dsdl.compile(
    '../tool/cyphalMessages',
    output_directory='../generated'
)
'''



# generate py files for yakut from dsdl message definition
yakut compile ../public_regulated_data_types/uavcan ./tool/cyphalMessages --output=./generated/cyphalHost