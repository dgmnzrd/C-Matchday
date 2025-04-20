// public/js/app.js
document.addEventListener('DOMContentLoaded', () => {
    // Inyectar footer
    fetch('/layouts/footer.html')
        .then(r => r.text())
        .then(html => {
            document.getElementById('footer-container').innerHTML = html;
        })
        .catch(console.error);
});