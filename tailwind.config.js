// tailwind.config.js
module.exports = {
    content: [
        "./public/**/*.html",
        "./public/js/**/*.js"
    ],
    theme: {
        extend: {
            colors: {
                primary: {
                    light: '#9d4edd',
                    DEFAULT: '#7b2cbf',
                    dark: '#5a189a'
                }
            }
        }
    },
    plugins: []
}