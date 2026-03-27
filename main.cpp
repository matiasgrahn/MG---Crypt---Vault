#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdio>
#include <iomanip>  
#include <sstream>  

const std::string TARKISTUS_SANA = "MAGIC_OK";

// Salausfunktio
bool kasitteleTiedosto(std::string syote, std::string vaste, std::string avain, bool salataanko) {
    if (avain.empty()) return false;
    std::ifstream eTiedosto(syote, std::ios::binary);
    if (!eTiedosto) return false;
    std::vector<char> data((std::istreambuf_iterator<char>(eTiedosto)), std::istreambuf_iterator<char>());
    eTiedosto.close();
    if (!salataanko) {
        if (data.size() < TARKISTUS_SANA.length()) return false;
        for (size_t i = 0; i < TARKISTUS_SANA.length(); ++i) {
            char purettuMerkki = data[i] ^ avain[i % avain.length()];
            if (purettuMerkki != TARKISTUS_SANA[i]) return false;
        }
        data.erase(data.begin(), data.begin() + TARKISTUS_SANA.length());
    }
    for (size_t i = 0; i < data.size(); ++i) {
        data[i] ^= avain[i % avain.length()];
    }
    std::ofstream vTiedosto(vaste, std::ios::binary);
    if (!vTiedosto) return false;
    if (salataanko) {
        for (size_t i = 0; i < TARKISTUS_SANA.length(); ++i) {
            char salattuTarkistus = TARKISTUS_SANA[i] ^ avain[i % avain.length()];
            vTiedosto.put(salattuTarkistus);
        }
    }
    vTiedosto.write(data.data(), data.size());
    vTiedosto.close();
    return true;
}

int main() {
    if (!glfwInit()) return 1;
    // Tehdään ikkunasta hieman korkeampi, jotta kaikki mahtuu
    GLFWwindow* window = glfwCreateWindow(750, 700, "MG-CRYPT v3.7 - Countdown Edition", NULL, NULL);
    if (!window) return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // UI-muuttujat
    static char tiedostoPolku[128] = "viesti.txt";
    static char salasana[128] = "";
    static char viestiSisalto[2048] = "";
    static std::string tilaViesti = "Järjestelmä valmiina.";
    static ImVec4 tilaVari = ImVec4(1, 1, 1, 1);
    static bool naytaKirjoitusBoxi = true;

    // Sulkemislogiikan muuttujat
    static bool suljetaankoOhjelma = false;
    static double sulkemisAika = 0;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("MG-CRYPT Hallintapaneeli", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

        // --- OSA 1: KIRJOITUS ---
        if (naytaKirjoitusBoxi) {
            ImGui::Text("1. KIRJOITA TAI MUOKKAA VIESTIA");
            ImGui::InputTextMultiline("##ViestiBox", viestiSisalto, IM_ARRAYSIZE(viestiSisalto), ImVec2(-FLT_MIN, 150));
            
            if (ImGui::Button("TALLENNA JA PIILOTA BOXI", ImVec2(300, 30))) {
                std::ofstream oTiedosto(tiedostoPolku);
                if (oTiedosto) {
                    oTiedosto << viestiSisalto;
                    oTiedosto.close();
                    tilaViesti = "Tallennettu: " + std::string(tiedostoPolku);
                    tilaVari = ImVec4(0, 0.8f, 1, 1);
                    naytaKirjoitusBoxi = false;
                }
            }

            // HASH-NAPPI AJASTIMELLA
            if (ImGui::Button("GENEROI HASH JA SULJE OHJELMA (5s)", ImVec2(-FLT_MIN, 30))) {
                std::string teksti(viestiSisalto);
                if (!teksti.empty()) {
                    std::size_t h = std::hash<std::string>{}(teksti);
                    std::stringstream ss;
                    ss << "HASH-TUNNISTE: 0x" << std::hex << std::uppercase << h;
                    strncpy(viestiSisalto, ss.str().c_str(), sizeof(viestiSisalto));
                    
                    suljetaankoOhjelma = true;
                    sulkemisAika = glfwGetTime() + 5.0; // Asetetaan 5 sekunnin päämäärä
                }
            }
        }

        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

        // --- OSA 2: ASETUKSET ---
        ImGui::Text("2. ASETUKSET");
        ImGui::InputText("Tiedoston nimi", tiedostoPolku, IM_ARRAYSIZE(tiedostoPolku));
        ImGui::InputText("Salasana", salasana, IM_ARRAYSIZE(salasana), ImGuiInputTextFlags_Password);

        ImGui::Spacing();

        // --- OSA 3: SALAUS JA PURKU ---
        if (ImGui::Button("SALAA TIEDOSTO", ImVec2(220, 50))) {
            if (kasitteleTiedosto(tiedostoPolku, "salattu.bin", salasana, true)) {
                tilaViesti = "ONNISTUI: Tiedosto salattu onnistuneesti.";
                tilaVari = ImVec4(0, 1, 0, 1);
            } else {
                tilaViesti = "VIRHE: Tiedostoa ei loytynyt!";
                tilaVari = ImVec4(1, 0, 0, 1);
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("PURASALAUS JA AVAA", ImVec2(220, 50))) {
            if (kasitteleTiedosto("salattu.bin", "purettu.txt", salasana, false)) {
                tilaViesti = "ONNISTUI: Salaus purettu ja boxi avattu!";
                tilaVari = ImVec4(0, 1, 0, 1);
                naytaKirjoitusBoxi = true;
                
                std::ifstream iTiedosto("purettu.txt");
                if (iTiedosto) {
                    std::string str((std::istreambuf_iterator<char>(iTiedosto)), std::istreambuf_iterator<char>());
                    strncpy(viestiSisalto, str.c_str(), sizeof(viestiSisalto));
                    iTiedosto.close();
                }
            } else {
                tilaViesti = "VIRHE: Vaara salasana!";
                tilaVari = ImVec4(1, 0, 0, 1);
            }
        }

        ImGui::Spacing(); ImGui::Separator();
        
        // --- OSA 4: SIIVOUS ---
        ImGui::Text("3. TIEDOSTOJEN HALLINTA");
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.2f, 0.2f, 1.0f));
        
        std::string poistoTeksti = "POISTA " + std::string(tiedostoPolku);
        if (ImGui::Button(poistoTeksti.c_str(), ImVec2(230, 30))) {
            remove(tiedostoPolku);
            tilaViesti = "Tiedosto poistettu.";
        }
        ImGui::SameLine();
        if (ImGui::Button("POISTA SALATTU.BIN", ImVec2(200, 30))) remove("salattu.bin");
        ImGui::SameLine();
        if (ImGui::Button("POISTA PURETTU.TXT", ImVec2(200, 30))) remove("purettu.txt");
        ImGui::PopStyleColor();

        ImGui::Spacing(); ImGui::Separator();

        // --- TILAVIESTI JA SULKEMISLASKURI ---
        if (suljetaankoOhjelma) {
            double jaljella = sulkemisAika - glfwGetTime();
            if (jaljella > 0) {
                tilaViesti = "HASH LUOTU! Ohjelma sulkeutuu: " + std::to_string((int)jaljella + 1) + "s";
                tilaVari = ImVec4(1.0f, 0.5f, 0.0f, 1.0f);
            } else {
                glfwSetWindowShouldClose(window, true);
            }
        }

        ImGui::Text("Tila:");
        ImGui::TextColored(tilaVari, "%s", tilaViesti.c_str());

        ImGui::End();

        // Renderointi
        ImGui::Render();
        glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // Siivous
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}