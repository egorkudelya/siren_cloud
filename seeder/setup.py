from setuptools import setup
setup(
    name='seeder',
    packages=['seeder'],
    version='0.1',
    entry_points={
        'console_scripts': [
            'seed = seeder.script:main',
        ]
    },
    python_requires='>=3.7',
    install_requires=[
        'requests',
        'pandas'
    ]
)