// Define a function that makes the GET request and creates the select field
function addSettingsPicker() {
  // Make a GET request to the server
  const settingsField = document.getElementById('settings')
  fetch("/settings")
    .then((response) => response.json())
    .then((data) => {
      // Create a select field
      const selectField = document.createElement("select");
      selectField.addEventListener("change", (event) => {
        const selectedSetting = event.target.value;
        if (document.contains(document.getElementById("settings-form"))) {
          document.getElementById("settings-form").remove();
        }
        if (selectedSetting == "Settings") {
          return;
        }
        fetch(`/settings/${selectedSetting}`)
          .then((response) => response.json())
          .then((data) => {
            const inputDiv = document.createElement("div");
            inputDiv.id = "settings-form";
            const inputField = document.createElement("input");
            inputField.type = "number";
            inputField.step = "any";
            inputField.value = data;
            inputDiv.appendChild(inputField)

            const button = document.createElement("button");
            button.textContent = "Set";
            button.addEventListener("click", () => {
              // When the button is clicked, send a post request
              const inputValue = inputField.value;
              const formData = new FormData();
              formData.append('value', inputValue);
              fetch(`/settings/${selectedSetting}`, {
                method: "POST",
                body: new URLSearchParams(formData),
              })
              .then((response) => {
                if (response.status === 200) {
                    settingsField.classList.add("success");
                    setTimeout(() => {
                        settingsField.classList.remove("success");
                    }, 300)
                }
              })
                .catch((error) => {
                  console.error("Error sending post request:", error);
                });
            });
            inputDiv.appendChild(button);
            settingsField.appendChild(inputDiv);
          });
      });

      // Add an option for each entry in the response
      const noneOption = document.createElement("option");
      noneOption.value = "Settings";
      noneOption.text = "Settings...";
      selectField.add(noneOption);
      data.forEach((entry) => {
        const option = document.createElement("option");
        option.value = entry;
        option.text = entry;
        selectField.add(option);
      });

      // Add the select field to the DOM
      document.getElementById("settings").appendChild(selectField);
    })
    .catch((error) => {
      // Handle errors
      console.error("Error fetching data:", error);
    });
}

// Run the function when the page loads
window.onload = addSettingsPicker;
