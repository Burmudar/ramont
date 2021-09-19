const path = require('path');

module.exports = {
    entry: './ts/src/app.ts',
    devtool: 'inliine-source-map',
    mode: "development",
    module: {
        rules: [{
            test: /\.tsx?/,
            use: 'ts-loader',
            exclude: '/node_modules/'
        }]
    },
    resolve: {
        extensions: ['.ts', '.js', '.tsx']
    },
    output: {
        filename: 'app-bundle.js',
        path: path.resolve(__dirname, 'static', 'dist')
    }
};
